#include "bzard_brightness_monitor.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <cmath>
#include <expected>
#include <optional>
// #include <stdexcept>
#include <string> // Оставляем для std::string

// --- Static Methods ---

/*static*/ QStringList
BzardBrightnessMonitor::availableDevices() { /* без изменений */
	const QDir BACKLIGHT_DIR(QStringLiteral("/sys/class/backlight"));
	if (!BACKLIGHT_DIR.exists()) {
		qWarning() << Q_FUNC_INFO
				   << "Directory /sys/class/backlight does not exist.";
		return QStringList();
	}
	return BACKLIGHT_DIR.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
	                               QDir::Name);
}

/*static*/ std::optional<BzardBrightnessMonitor::DevicePaths>
BzardBrightnessMonitor::findDevicePaths(
	  const QString &DEVICE_NAME) { /* без изменений */
	if (DEVICE_NAME.isEmpty()) {
		qWarning() << Q_FUNC_INFO << "Device name cannot be empty.";
		return std::nullopt;
	}
	const QDir BACKLIGHT_DIR(QStringLiteral("/sys/class/backlight"));
	DevicePaths paths;
	paths.basePath = BACKLIGHT_DIR.filePath(DEVICE_NAME);
	const QFileInfo BASE_INFO(paths.basePath);
	if (!BASE_INFO.exists() || !BASE_INFO.isDir()) {
		qWarning() << Q_FUNC_INFO
				   << "Device directory not found or is not a directory:"
				   << paths.basePath;
		return std::nullopt;
	}
	paths.brightnessFilePath = BASE_INFO.filePath() + QDir::separator() +
	                           QStringLiteral("brightness");
	paths.maxBrightnessFilePath = BASE_INFO.filePath() + QDir::separator() +
	                              QStringLiteral("max_brightness");
	paths.canonicalSysPath = BASE_INFO.canonicalFilePath();
	bool files_ok = QFile::exists(paths.brightnessFilePath) &&
	                QFile::exists(paths.maxBrightnessFilePath) &&
	                !paths.canonicalSysPath.isEmpty();
	if (!files_ok) {
		qWarning() << Q_FUNC_INFO
				   << "Required files or syspath not found/valid for device"
				   << DEVICE_NAME << "at" << paths.basePath;
		return std::nullopt;
	}
	return paths;
}

// --- Constructor ---
BzardBrightnessMonitor::BzardBrightnessMonitor(
	  const DevicePaths &PATHS, QObject *parent) /* без изменений */
	  : QObject(parent), BASE_PATH(PATHS.basePath),
		BRIGHTNESS_FILE_PATH(PATHS.brightnessFilePath),
		MAX_BRIGHTNESS_FILE_PATH(PATHS.maxBrightnessFilePath),
		CANONICAL_SYS_PATH(PATHS.canonicalSysPath) {
	if (BASE_PATH.isEmpty() || BRIGHTNESS_FILE_PATH.isEmpty() ||
	    MAX_BRIGHTNESS_FILE_PATH.isEmpty() || CANONICAL_SYS_PATH.isEmpty()) {
		qCritical() << Q_FUNC_INFO << "FATAL: Constructed with invalid paths!";
	}
	checkForUpdate();
}

QString BzardBrightnessMonitor::sysPath() const { /* без изменений */
	return CANONICAL_SYS_PATH;
}

int64_t BzardBrightnessMonitor::readLongFromFile(const QString &FILE_PATH) {
	QFile file(FILE_PATH);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw QString("Could not open file %1: %2")
			  .arg(FILE_PATH, file.errorString())
			  .toStdString();
	}
	bool ok = false;
	const QString CONTENT = file.readAll().trimmed();
	qlonglong value = CONTENT.toLongLong(&ok);
	if (!ok) {
		throw QString("Failed to parse value '%1' from file: %2")
			  .arg(CONTENT, FILE_PATH)
			  .toStdString();
	}
	return value;
}

void BzardBrightnessMonitor::checkForUpdate() { /* без изменений */
	std::expected<qlonglong, std::string> currentRes =
		  readLongFromFile(BRIGHTNESS_FILE_PATH);
	std::expected<qlonglong, std::string> maxRes =
		  readLongFromFile(MAX_BRIGHTNESS_FILE_PATH);

	if (!currentRes) {
		qWarning() << Q_FUNC_INFO << QString::fromStdString(currentRes.error());
		return;
	}
	if (!maxRes) {
		qWarning() << Q_FUNC_INFO << QString::fromStdString(maxRes.error());
		return;
	}

	const qlonglong CURRENT = currentRes.value();
	const qlonglong MAX = maxRes.value();

	if (MAX <= 0) {
		qWarning() << Q_FUNC_INFO << "Invalid max_brightness value (" << MAX
				   << ") for" << BASE_PATH;
		return;
	}

	const int CURRENT_PERCENT = std::max(
		  0, std::min(100, static_cast<int>(std::round(
								 static_cast<double>(CURRENT) * 100.0 / MAX))));

	if (CURRENT_PERCENT != lastBrightnessPercent) {
		lastBrightnessPercent = CURRENT_PERCENT;
		emit brightnessChanged(lastBrightnessPercent);
	}
}
