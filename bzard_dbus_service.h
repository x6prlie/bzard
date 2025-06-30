/*
 *     This file is part of bzard.
 *
 * bzard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * bzard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bzard.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>

#include <QObject>
#include <QString>
#include <QStringList>

#include "bzard_config.h"
#include "bzard_notification_receiver.h"

class BzardDBusService : public QObject {
	Q_OBJECT

  public:
	static QString versionString();
	static QString appString();

	using QObject::QObject;

	BzardDBusService *connectReceiver(BzardNotificationReceiver *receiver);

	/*
	 * For configurable modifiers we should check is it enabled in config
	 */
	template <class T>
	typename std::enable_if_t<std::is_base_of<BzardConfigurable, T>::value,
	                          BzardDBusService *>
	addModifier(std::unique_ptr<T> modifier) {
		if (modifier->isEnabled())
			modifers.push_back(std::move(modifier));
		return this;
	}

	template <class T>
	typename std::enable_if_t<!std::is_base_of<BzardConfigurable, T>::value,
	                          BzardDBusService *>
	addModifier(std::unique_ptr<T> modifier) {
		modifers.push_back(std::move(modifier));
		return this;
	}

	// DBus interface
	QStringList getCapabilities();

	QString getServerInformation(QString &vendor, QString &version,
	                             QString &specVersion);

	uint32_t notify(const QString &APP_NAME, uint32_t replacesId,
	                const QString &APP_ICON, const QString &SUMMARY,
	                const QString &BODY, const QStringList &ACTIONS,
	                const QVariantMap &HINTS, uint32_t expireTimeout);

	void closeNotification(uint32_t id);

  signals:
	// DBus signals
	void actionInvoked(uint32_t notificationId, const QString &ACTION_KEY);

	void notificationClosed(uint32_t notificationId, uint32_t reason);

	// Internal signals
	void createNotificationSignal(const BzardNotification &NOTIFICATON);
	void dropNotificationSignal(BzardNotification::id_t id);

  public slots:
	void onNotificationDropped(BzardNotification::id_t id,
	                           BzardNotification::ClosingReason reason);
	void onActionInvoked(BzardNotification::id_t id, const QString &ACTION_KEY);

  private:
	std::vector<BzardNotificationModifier::ptr_t> modifers;

	BzardNotification modify(BzardNotification NOTIFICATON);
};
