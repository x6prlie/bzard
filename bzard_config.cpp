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

#include "bzard_config.h"

#include <qt6xdg/XdgDirs>

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {
// TODO: refactor; taken from here: https://gist.github.com/ssendeavour/7324701
static bool copyRecursively(const QString &SRC_FILE_PATH,
                            const QString &TGT_FILE_PATH) {
	QFileInfo srcFileInfo(SRC_FILE_PATH);
	if (srcFileInfo.isDir()) {
		QDir targetDir(TGT_FILE_PATH);
		targetDir.cdUp();
		if (!targetDir.mkdir(QFileInfo(TGT_FILE_PATH).fileName()))
			return false;
		QDir sourceDir(SRC_FILE_PATH);
		QStringList fileNames = sourceDir.entryList(
			  QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden |
			  QDir::System);
		for (const QString &fileName : fileNames) {
			const QString NEW_SRC_FILE_PATH =
				  SRC_FILE_PATH + QLatin1Char('/') + fileName;
			const QString NEW_TGT_FILE_PATH =
				  TGT_FILE_PATH + QLatin1Char('/') + fileName;
			if (!copyRecursively(NEW_SRC_FILE_PATH, TGT_FILE_PATH))
				return false;
		}
	} else {
		if (!QFile::copy(SRC_FILE_PATH, TGT_FILE_PATH)) // NOLINT
			return false;
	}
	return true;
}
} // namespace

BzardConfig::BzardConfig(const QString &CATEGORY_, const QString &FILE_NAME_)
	  : CATEGORY{CATEGORY_.isEmpty() ? "" : CATEGORY_ + '/'},
		FILE_NAME{FILE_NAME_},
		settings{std::make_unique<QSettings>(getConfigFileName(),
                                             QSettings::IniFormat)} {
	qDebug() << getConfigFileName();
}

QVariant BzardConfig::value(const QString &KEY,
                            const QVariant &DEFAULT_VALUE) const {
	return settings->value(CATEGORY + KEY, DEFAULT_VALUE);
}

void BzardConfig::setValue(const QString &KEY, const QVariant &VALUE) {
	settings->setValue(CATEGORY + KEY, VALUE);
}

#define IQ_MACRO_STRING(S) IQ_MACRO_STRING__(S)
#define IQ_MACRO_STRING__(S) #S
QString BzardConfig::applicationName() {
	return QStringLiteral(IQ_MACRO_STRING(IQ_APP_NAME));
}

QString BzardConfig::configDir() {
	static auto configDir = XdgDirs::configHome() + '/' + applicationName();
	return configDir;
}

QString BzardConfig::applicationVersion() {
	return QStringLiteral(IQ_MACRO_STRING(IQ_VERSION));
}
#undef IQ_MACRO_STRING__
#undef IQ_MACRO_STRING

QString BzardConfig::getConfigFileName() const {
	auto config = configDir() + '/' + FILE_NAME;
	QFileInfo config_file{config};

	if (config_file.exists()) {
		if (!config_file.isFile())
			throw std::runtime_error{config.toStdString() +
			                         " is not a valid config file"};
	} else {
		QDir dir;
		dir.mkpath(configDir());
		if (config.contains("/themes/default/theme"))
			copyThemesFromShare(QString{config}.replace("/default/theme", ""));
		else
			copyConfigFileFromExample(config);
	}
	return config;
}

bool BzardConfig::copyConfigFileFromExample(const QString &destination) const {
	auto config_example_path =
		  "/usr/share/" + applicationName() + '/' + FILE_NAME + ".example";
	QFile config_example_file{config_example_path};
	if (!config_example_file.exists())
		return false;
	return config_example_file.copy(destination);
}

bool BzardConfig::copyThemesFromShare(const QString &destination) const {
	auto shareThemesPath = "/usr/share/" + applicationName() + "/themes";
	return copyRecursively(shareThemesPath, destination);
}

BzardConfigurable::BzardConfigurable(const QString &NAME)
	  : NAME_{NAME}, config{NAME_} {}

const QString &BzardConfigurable::NAME() const { return NAME_; }

bool BzardConfigurable::isEnabled() const {
	return config.value("enabled", false).toBool();
}
