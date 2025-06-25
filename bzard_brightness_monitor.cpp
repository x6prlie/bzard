#include "bzard_brightness_monitor.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <cmath>
#include <expected>
#include <optional>
#include <stdexcept>
#include <string> // Оставляем для std::string

// --- Static Methods ---

/*static*/ QStringList
BzardBrightnessMonitor::availableDevices() { /* без изменений */
	const QDir backlightDir(QStringLiteral("/sys/class/backlight"));
	if (!backlightDir.exists()) {
		qWarning() << Q_FUNC_INFO
				   << "Directory /sys/class/backlight does not exist.";
		return QStringList();
	}
	return backlightDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
	                              QDir::Name);
}

/*static*/ std::optional<BzardBrightnessMonitor::DevicePaths>
BzardBrightnessMonitor::findDevicePaths(
	  const QString &deviceName) { /* без изменений */
	if (deviceName.isEmpty()) {
		qWarning() << Q_FUNC_INFO << "Device name cannot be empty.";
		return std::nullopt;
	}
	const QDir backlightDir(QStringLiteral("/sys/class/backlight"));
	DevicePaths paths;
	paths.basePath = backlightDir.filePath(deviceName);
	const QFileInfo baseInfo(paths.basePath);
	if (!baseInfo.exists() || !baseInfo.isDir()) {
		qWarning() << Q_FUNC_INFO
				   << "Device directory not found or is not a directory:"
				   << paths.basePath;
		return std::nullopt;
	}
	paths.brightnessFilePath = baseInfo.filePath() + QDir::separator() +
	                           QStringLiteral("brightness");
	paths.maxBrightnessFilePath = baseInfo.filePath() + QDir::separator() +
	                              QStringLiteral("max_brightness");
	paths.canonicalSysPath = baseInfo.canonicalFilePath();
	bool files_ok = QFile::exists(paths.brightnessFilePath) &&
	                QFile::exists(paths.maxBrightnessFilePath) &&
	                !paths.canonicalSysPath.isEmpty();
	if (!files_ok) {
		qWarning() << Q_FUNC_INFO
				   << "Required files or syspath not found/valid for device"
				   << deviceName << "at" << paths.basePath;
		return std::nullopt;
	}
	return paths;
}

// --- Constructor ---
BzardBrightnessMonitor::BzardBrightnessMonitor(
	  const DevicePaths &paths, QObject *parent) /* без изменений */
	  : QObject(parent), basePath(paths.basePath),
		brightnessFilePath(paths.brightnessFilePath),
		maxBrightnessFilePath(paths.maxBrightnessFilePath),
		canonicalSysPath(paths.canonicalSysPath) {
	if (basePath.isEmpty() || brightnessFilePath.isEmpty() ||
	    maxBrightnessFilePath.isEmpty() || canonicalSysPath.isEmpty()) {
		qCritical() << Q_FUNC_INFO << "FATAL: Constructed with invalid paths!";
	}
	checkForUpdate();
}

QString BzardBrightnessMonitor::syspath() const { /* без изменений */
	return canonicalSysPath;
}

int64_t BzardBrightnessMonitor::readLongFromFile(const QString &filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw QString("Could not open file %1: %2")
			  .arg(filePath, file.errorString())
			  .toStdString();
	}
	bool ok = false;
	const QString content = file.readAll().trimmed();
	qlonglong value = content.toLongLong(&ok);
	if (!ok) {
		throw QString("Failed to parse value '%1' from file: %2")
			  .arg(content, filePath)
			  .toStdString();
	}
	return value;
}

void BzardBrightnessMonitor::checkForUpdate() { /* без изменений */
	std::expected<qlonglong, std::string> currentRes = readLongFromFile(brightnessFilePath);
	std::expected<qlonglong, std::string> maxRes = readLongFromFile(maxBrightnessFilePath);

	if (!currentRes) {
		qWarning() << Q_FUNC_INFO << QString::fromStdString(currentRes.error());
		return;
	}
	if (!maxRes) {
		qWarning() << Q_FUNC_INFO << QString::fromStdString(maxRes.error());
		return;
	}

	const qlonglong current = currentRes.value();
	const qlonglong max = maxRes.value();

	if (max <= 0) {
		qWarning() << Q_FUNC_INFO << "Invalid max_brightness value (" << max
				   << ") for" << basePath;
		return;
	}

	const int currentPercent = std::max(
		  0, std::min(100, static_cast<int>(std::round(
								 static_cast<double>(current) * 100.0 / max))));

	if (currentPercent != lastBrightnessPercent) {
		lastBrightnessPercent = currentPercent;
		emit brightnessChanged(lastBrightnessPercent);
	}
}
