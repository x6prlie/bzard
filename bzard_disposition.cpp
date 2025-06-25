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

#include "bzard_disposition.h"

#include <QApplication>

BzardDisposition::BzardDisposition(QObject *parent)
	  : QObject{parent}, SCREEN_{QApplication::screens().at(0)} {
	connect(SCREEN_, &QScreen::availableGeometryChanged, this,
	        &BzardDisposition::recalculateAvailableScreenGeometry);
}

const QScreen *BzardDisposition::SCREEN() const { return SCREEN_; }

void BzardDisposition::setExtraWindowSize(const QSize &VALUE) {
	extraWindowSize = VALUE;
}

void BzardDisposition::setMargins(const QMargins &VALUE) {
	margins = VALUE;
	recalculateAvailableScreenGeometry();
}

void BzardDisposition::setSpacing(int value) { spacing = value; }

void BzardDisposition::recalculateAvailableScreenGeometry() {
	auto screenGeometry = SCREEN()->availableGeometry();
	availableScreenGeometry = screenGeometry - margins;
}
