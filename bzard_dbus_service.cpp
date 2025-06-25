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

#include "bzard_dbus_service.h"

#include "bzard_config.h"

QString BzardDBusService::versionString() {
	return BzardConfig::applicationVersion();
}

QString BzardDBusService::appString() { return BzardConfig::applicationName(); }

BzardDBusService *
BzardDBusService::connectReceiver(BzardNotificationReceiver *receiver) {
	connect(this, &BzardDBusService::createNotificationSignal, receiver,
	        &BzardNotificationReceiver::onCreateNotification);
	connect(this, &BzardDBusService::dropNotificationSignal, receiver,
	        &BzardNotificationReceiver::onDropNotification);
	connect(receiver, &BzardNotificationReceiver::actionInvokedSignal, this,
	        &BzardDBusService::onActionInvoked);
	connect(receiver, &BzardNotificationReceiver::notificationDroppedSignal,
	        this, &BzardDBusService::onNotificationDropped);
	return this;
}

QStringList BzardDBusService::getCapabilities() {
	auto caps = QStringList{} << "actions"
	                          // << "action-icons"
	                          << "body"
	                          // << "body-hyperlinks"
	                          // << "body-images"
	                          << "body-markup"
	                          // << "icon-multi"
	                          << "icon-static"
	                          << "persistence"
		  // << "sound"
		  ;
	return caps;
}

QString BzardDBusService::getServerInformation(QString &vendor,
                                               QString &version,
                                               QString &specVersion) {
	specVersion = QString("1.2");
	version = versionString();
	vendor = QString("bzard.project");
	return QString("bzard");
}

uint32_t BzardDBusService::notify(const QString &APP_NAME, uint32_t replaces_id,
                                  const QString &APP_ICON,
                                  const QString &SUMMARY, const QString &BODY,
                                  const QStringList &ACTIONS,
                                  const QVariantMap &HINTS,
                                  uint32_t expire_timeout) {
	auto notification = modify(
		  {replaces_id, APP_NAME, BODY, SUMMARY, APP_ICON, ACTIONS, HINTS,
	       static_cast<BzardNotification::ExpireTimeout>(expire_timeout),
	       replaces_id});
	emit createNotificationSignal(notification);
	return notification.id;
}

void BzardDBusService::closeNotification(uint32_t id) {
	emit dropNotificationSignal(id);
}

void BzardDBusService::onNotificationDropped(
	  BzardNotification::id_t id,
	  BzardNotification::ClosingReason /*!!!enum!!!*/ reason) {
	emit notificationClosed(id, reason);
}

void BzardDBusService::onActionInvoked(BzardNotification::id_t id,
                                       const QString &ACTION_KEY) {
	emit actionInvoked(id, ACTION_KEY);
}

BzardNotification BzardDBusService::modify(BzardNotification notification) {
	qDebug() << "========*****========";
	qDebug() << notification;
	for (auto & /*!!!*/ m : modifers)
		m->modify(notification);
	qDebug() << notification;
	return notification;
}
