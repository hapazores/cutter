#include <Cutter.h>
#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "R2PluginsDialog.h"
#include "r_version.h"
#include "utils/Configuration.h"

#include <QUrl>
#include <QTimer>
#include <QJsonObject>
#include <QProgressBar>
#include <QProgressDialog>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>

#include "CutterConfig.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());

    ui->label->setText(tr("<h1>Cutter</h1>"
                          "Version " CUTTER_VERSION_FULL "<br/>"
                          "Using r2-" R2_GITTAP
                          "<p><b>Optional Features:</b><br/>"
                          "Jupyter: %1<br/>"
                          "QtWebEngine: %2</p>"
                          "<h2>License</h2>"
                          "This Software is released under the GNU General Public License v3.0"
                          "<h2>Authors</h2>"
                          "xarkes, thestr4ng3r, ballessay<br/>"
                          "Based on work by Hugo Teso &lt;hugo.teso@gmail.org&gt; (originally Iaito).")
                       .arg(
#ifdef CUTTER_ENABLE_JUPYTER
                           "ON"
#else
                           "OFF"
#endif
                           ,
#ifdef CUTTER_ENABLE_QTWEBENGINE
                           "ON"
#else
                           "OFF"
#endif
                       ));
}

AboutDialog::~AboutDialog() {}

void AboutDialog::on_buttonBox_rejected()
{
    close();
}

void AboutDialog::on_showVersionButton_clicked()
{
    QMessageBox popup(this);
    popup.setWindowTitle(tr("radare2 version information"));
    auto versionInformation = Core()->getVersionInformation();
    popup.setText(versionInformation);
    popup.exec();
}

void AboutDialog::on_showPluginsButton_clicked()
{
    R2PluginsDialog dialog(this);
    dialog.exec();
}

void AboutDialog::on_checkForUpdatesButton_clicked()
{
    QUrl url("https://api.github.com/repos/radareorg/cutter/releases/latest");
    QNetworkRequest request;
    request.setUrl(url);

    QProgressDialog waitDialog;
    QProgressBar *bar = new QProgressBar(&waitDialog);
    bar->setMaximum(0);

    waitDialog.setBar(bar);
    waitDialog.setLabel(new QLabel(tr("Checking for updates..."), &waitDialog));

    QNetworkAccessManager nm;

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(7000);

    connect(&nm, &QNetworkAccessManager::finished, &timeoutTimer, &QTimer::stop);
    connect(&nm, &QNetworkAccessManager::finished, &waitDialog, &QProgressDialog::cancel);
    connect(&nm, &QNetworkAccessManager::finished, this, &AboutDialog::serveVersionCheckReply);

    QNetworkReply *reply = nm.get(request);
    timeoutTimer.start();

    connect(&timeoutTimer, &QTimer::timeout, []() {
        QMessageBox mb;
        mb.setIcon(QMessageBox::Critical);
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setWindowTitle(tr("Timeout error!"));
        mb.setText(tr("Please check your internet connection and try again."));
        mb.exec();
    });

    waitDialog.exec();
    delete reply;
}

void AboutDialog::serveVersionCheckReply(QNetworkReply *reply)
{
    QString currVersion = "";
    QMessageBox mb;
    mb.setStandardButtons(QMessageBox::Ok);
    if (reply->error()) {
        mb.setIcon(QMessageBox::Critical);
        mb.setWindowTitle(tr("Error!"));
        mb.setText(reply->errorString());
    } else {
        currVersion = QJsonDocument::fromJson(reply->readAll()).object().value("tag_name").toString();
        currVersion.remove('v');

        mb.setWindowTitle(tr("Version control"));
        mb.setIcon(QMessageBox::Information);
        if (currVersion == CUTTER_VERSION_FULL) {
            mb.setText(tr("You have latest version and no need to update!"));
        } else {
            mb.setText(tr("<b>Current version</b>: " CUTTER_VERSION_FULL "<br/>"
                          "<b>Latest version</b>: %1<br/><br/>"
                          "For update, please check the link: <a href=\"%2\">%2</a>")
                       .arg(currVersion, "https://github.com/radareorg/cutter/releases"));
        }
    }
    mb.exec();
}
