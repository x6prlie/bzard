#include "bzard_udev_monitor.h"
#include <QDebug>
// Убираем format, так как используем QString::arg или QDebug
// #include <format>

UdevMonitor::UdevMonitor(QObject *parent) : QObject(parent) {}

UdevMonitor::~UdevMonitor() {
	if (udevNotifier) {
		udevNotifier->setEnabled(false);
	}
	if (udevMonitor) {
		udev_monitor_unref(udevMonitor);
	}
	if (udevContext) {
		udev_unref(udevContext);
	}
}

bool UdevMonitor::initialize() {
	if (udevContext) {
		qWarning() << Q_FUNC_INFO << "Already initialized.";
		return true;
	}

	udevContext = udev_new();
	if (!udevContext) {
		const QString ERROR_MESSAGE =
			  QStringLiteral("Failed to create udev context");
		qCritical() << Q_FUNC_INFO << ERROR_MESSAGE;
		emit errorOccurred(ERROR_MESSAGE);
		return false;
	}

	udevMonitor = udev_monitor_new_from_netlink(udevContext, "udev");
	if (!udevMonitor) {
		const QString ERROR_MESSAGE =
			  QStringLiteral("Failed to create udev monitor");
		qCritical() << Q_FUNC_INFO << ERROR_MESSAGE;
		emit errorOccurred(ERROR_MESSAGE);
		udev_unref(udevContext);
		udevContext = nullptr;
		return false;
	}

	if (udev_monitor_enable_receiving(udevMonitor) < 0) {
		const QString ERROR_MESSAGE =
			  QStringLiteral("Failed to enable udev receiving");
		qCritical() << Q_FUNC_INFO << ERROR_MESSAGE;
		emit errorOccurred(ERROR_MESSAGE);
		udev_monitor_unref(udevMonitor);
		udev_unref(udevContext);
		udevMonitor = nullptr;
		udevContext = nullptr;
		return false;
	}

	const int UDEV_FEED = udev_monitor_get_fd(udevMonitor);
	if (UDEV_FEED < 0) {
		const QString ERROR_MESSAGE =
			  QStringLiteral("Failed to get udev monitor FD");
		qCritical() << Q_FUNC_INFO << ERROR_MESSAGE;
		emit errorOccurred(ERROR_MESSAGE);
		udev_monitor_unref(udevMonitor);
		udev_unref(udevContext);
		udevMonitor = nullptr;
		udevContext = nullptr;
		return false;
	}

	udevNotifier = new QSocketNotifier(UDEV_FEED, QSocketNotifier::Read, this);
	connect(udevNotifier, &QSocketNotifier::activated, this,
	        &UdevMonitor::onUdevFdReadable);
	udevNotifier->setEnabled(true);
	qInfo() << Q_FUNC_INFO << "Udev monitoring initialized.";
	return true;
}

bool UdevMonitor::addSubsystemFilter(const QString &SUBSYSTEM) {
	if (!udevMonitor) {
		const QString ERROR_MESSAGE =
			  QStringLiteral("Cannot add filter: udev monitor not initialized");
		qWarning() << Q_FUNC_INFO << ERROR_MESSAGE;
		emit errorOccurred(ERROR_MESSAGE);
		return false;
	}

	if (udev_monitor_filter_add_match_subsystem_devtype(
			  udevMonitor, SUBSYSTEM.toUtf8().constData(), nullptr) < 0) {
		// Используем QString::arg для форматирования
		const QString ERROR_MESSAGE =
			  QString("Failed to add udev filter for subsystem %1.")
					.arg(SUBSYSTEM);
		qCritical() << Q_FUNC_INFO << ERROR_MESSAGE;
		emit errorOccurred(ERROR_MESSAGE);
		return false;
	}
	qInfo() << Q_FUNC_INFO << "Added filter for subsystem:" << SUBSYSTEM;
	return true;
}

void UdevMonitor::onUdevFdReadable() {
	if (!udevMonitor)
		return;
	udev_device *dev = udev_monitor_receive_device(udevMonitor);
	if (!dev) {
		return;
	}
	const char *ACTION_C_STR = udev_device_get_action(dev);
	const char *SYS_PATH_C_STR = udev_device_get_syspath(dev);
	emit udevEvent(ACTION_C_STR ? QString::fromUtf8(ACTION_C_STR) : QString(),
	               SYS_PATH_C_STR ? QString::fromUtf8(SYS_PATH_C_STR)
	                              : QString());
	udev_device_unref(dev);
}
