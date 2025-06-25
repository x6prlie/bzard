#include "bzard_qml_brightness_monitor.h"
#include <QDebug>

QMLBrightnessMonitor::QMLBrightnessMonitor(QObject *parent) : QObject(parent) {
	qInfo() << Q_FUNC_INFO << "Singleton instance created.";
}

bool QMLBrightnessMonitor::initialize() {
	if (isInitialized) {
		return true;
	}
	qInfo() << Q_FUNC_INFO << "Initializing...";

	udevMonitorInstance = std::make_unique<UdevMonitor>();
	if (!udevMonitorInstance->initialize()) {
		emit errorOccurred("UdevMonitor", "Initialization failed.");
		udevMonitorInstance.reset();
		return false;
	}
	if (!udevMonitorInstance->addSubsystemFilter(QStringLiteral("backlight"))) {
		emit errorOccurred("UdevMonitor",
		                   "Failed to add backlight subsystem filter.");
		udevMonitorInstance.reset();
		return false;
	}
	connect(udevMonitorInstance.get(), &UdevMonitor::udevEvent, this,
	        &QMLBrightnessMonitor::handleUdevEvent);
	connect(udevMonitorInstance.get(), &UdevMonitor::errorOccurred, this,
	        &QMLBrightnessMonitor::onUdevError);

	setupBrightnessMonitors();

	isInitialized = true;
	qInfo() << Q_FUNC_INFO << "Initialization complete.";
	return true;
}

void QMLBrightnessMonitor::setupBrightnessMonitors() {
	qInfo() << Q_FUNC_INFO << "Setting up brightness monitors...";
	// Удаляем предыдущие мониторы (если были), Qt позаботится об объектах
	qDeleteAll(
		  activeMonitors); // Удаляем старые мониторы, если это реинициализация
	activeMonitors.clear();
	syspathToDeviceName.clear();
	internalBrightnessLevels.clear();

	internalAvailableDevices = BzardBrightnessMonitor::availableDevices();
	qInfo() << "Found devices:" << internalAvailableDevices;

	bool ok = false;
	for (const QString &DEVICE_NAME : internalAvailableDevices) {
		std::optional<BzardBrightnessMonitor::DevicePaths> pathsOpt =
			  BzardBrightnessMonitor::findDevicePaths(DEVICE_NAME);
		if (!pathsOpt) {
			emit errorOccurred(DEVICE_NAME, "Could not find/validate paths.");
			continue;
		}

		try {
			BzardBrightnessMonitor::DevicePaths paths = pathsOpt.value();
			// --- Создаем монитор как дочерний объект QMLBrightnessMonitor ---
			BzardBrightnessMonitor *monitor = new BzardBrightnessMonitor(
				  paths, this); // `this` как родитель

			connect(monitor,
			        &BzardBrightnessMonitor::brightnessChanged, // Используем
			                                                    // raw pointer
			        this, &QMLBrightnessMonitor::onDeviceBrightnessChanged);

			QString syspath = monitor->syspath();
			syspathToDeviceName[syspath] = DEVICE_NAME;

			// --- Вставляем raw pointer в QMap ---
			activeMonitors.insert(syspath, monitor);
			// -----------------------------------

			ok = true;

		} catch (const std::exception &EXCEPTION) {
			emit errorOccurred(DEVICE_NAME,
			                   QString("Exception during creation: %1")
			                         .arg(EXCEPTION.what()));
		}
	}

	if (!ok) {
		qWarning() << Q_FUNC_INFO
				   << "Failed to set up any brightness monitors.";
	}
	emit brightnessLevelsChanged();
}

// Геттеры без изменений
QStringList QMLBrightnessMonitor::availableDevices() const {
	return internalAvailableDevices;
}
QVariantMap QMLBrightnessMonitor::brightnessLevels() const {
	return internalBrightnessLevels;
}

// Слоты без изменений в логике, но используют BrightnessMonitor*
void QMLBrightnessMonitor::handleUdevEvent(const QString &ACTION,
                                           const QString &EVENT_SYS_PATH) {
	if (ACTION != QLatin1String("change") || EVENT_SYS_PATH.isEmpty()) {
		return;
	}
	// Итерация по QMap<QString, BrightnessMonitor*>
	for (auto it = activeMonitors.constBegin(); it != activeMonitors.constEnd();
	     ++it) {
		const QString &monitorSyspath = it.key();
		BzardBrightnessMonitor *monitorPtr = it.value(); // Получаем raw pointer
		if (EVENT_SYS_PATH.startsWith(monitorSyspath) && monitorPtr) {
			monitorPtr->checkForUpdate();
			break;
		}
	}
}

void QMLBrightnessMonitor::onDeviceBrightnessChanged(int brightnessPercent) {
	BzardBrightnessMonitor *senderMonitor =
		  qobject_cast<BzardBrightnessMonitor *>(sender());
	if (!senderMonitor) {
		return;
	}
	QString senderSyspath = senderMonitor->syspath();
	QString deviceName = syspathToDeviceName.value(
		  senderSyspath, QStringLiteral("UnknownDevice"));
	bool changed = false;
	if (!internalBrightnessLevels.contains(deviceName) ||
	    internalBrightnessLevels.value(deviceName).toInt() !=
	          brightnessPercent) {
		internalBrightnessLevels[deviceName] = brightnessPercent;
		changed = true;
	}
	if (changed) {
		emit brightnessLevelsChanged();
	}
}

void QMLBrightnessMonitor::onUdevError(const QString &ERROR_STRING) {
	qWarning() << "[UdevMonitor Error]" << ERROR_STRING;
	emit errorOccurred(QStringLiteral("UdevMonitor"), ERROR_STRING);
}

// --- Синглтон-провайдер (без изменений) ---
static QPointer<QMLBrightnessMonitor> g_brightnessMonitorInstance = nullptr;
QObject *qml_brightness_monitor_provider(QQmlEngine *engine,
                                         QJSEngine *scriptEngine) {
	Q_UNUSED(engine);
	Q_UNUSED(scriptEngine);
	if (!g_brightnessMonitorInstance) {
		g_brightnessMonitorInstance = new QMLBrightnessMonitor();
		if (!g_brightnessMonitorInstance->initialize()) {
			qCritical("QMLBrightnessMonitor failed to initialize!");
			return nullptr;
		}
	}
	return g_brightnessMonitorInstance;
}
