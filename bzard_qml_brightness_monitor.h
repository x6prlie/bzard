#pragma once

#include <QJSEngine>
#include <QMap>
#include <QObject>
#include <QQmlEngine>
#include <QStringList>
// #include <memory> // Больше не нужен для QMap

#include "bzard_brightness_monitor.h"
#include "bzard_udev_monitor.h"

class QMLBrightnessMonitor : public QObject {
	Q_OBJECT
	Q_PROPERTY(QStringList availableDevices READ availableDevices CONSTANT)
	Q_PROPERTY(QVariantMap brightnessLevels READ brightnessLevels NOTIFY
	                 brightnessLevelsChanged)

  public:
	explicit QMLBrightnessMonitor(QObject *parent = nullptr);
	// Деструктор по умолчанию ОК, Qt удалит дочерние объекты
	~QMLBrightnessMonitor() override = default;

	bool initialize();
	QStringList availableDevices() const;
	QVariantMap brightnessLevels() const;

  signals:
	void brightnessLevelsChanged();
	void errorOccurred(const QString &ERROR_SOURCE,
	                   const QString &ERROR_MESSAGE);

  private slots:
	void handleUdevEvent(const QString &ACTION, const QString &EVENT_SYS_PATH);
	void onDeviceBrightnessChanged(int brightnessPercent);
	void onUdevError(const QString &ERROR_STRING);

  private:
	// --- Тип карты изменен на raw pointer ---
	// Мы делаем BzardBrightnessMonitor дочерними объектами этого класса,
	// поэтому Qt автоматически удалит их при удалении QMLBrightnessMonitor.
	using MonitorMap = QMap<QString, BzardBrightnessMonitor *>;

	void setupBrightnessMonitors();

	QStringList internalAvailableDevices;
	QVariantMap internalBrightnessLevels;
	MonitorMap activeMonitors; // Теперь хранит raw pointers
	QMap<QString, QString> sysPathToDeviceName;

	// unique_ptr все еще подходит для UdevMonitor, так как он один
	std::unique_ptr<UdevMonitor> udevMonitorInstance;
	bool isInitialized = false;
};

QObject *qmlBrightnessMonitorProvider(QQmlEngine *engine,
                                      QJSEngine *scriptEngine);
