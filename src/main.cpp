#include <QApplication>
#include "MainWindow.h"
#include "DarkTheme.h"
#include "AppConstants.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(AppConstants::AppName);
    app.setApplicationVersion(AppConstants::AppVersion);
    app.setOrganizationName(AppConstants::OrgName);

    DarkTheme::apply(app);

    MainWindow window;
    window.show();

    return app.exec();
}
