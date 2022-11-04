#include "mainwindow.h"
#include <QApplication>

#if defined (Q_OS_ANDROID)
#include <QtAndroidExtras/QtAndroid>

bool requestAndroidPermissions(){
    //Request requiered permissions at runtime

    const QVector<QString> permissions({/*"android.permission.ACCESS_COARSE_LOCATION",*/
                                        "android.permission.INTERNET",
                                        "android.permission.WRITE_EXTERNAL_STORAGE",
                                        "android.permission.READ_EXTERNAL_STORAGE"});

    for(const QString &permission : permissions){
        auto result = QtAndroid::checkPermission(permission);
        if(result == QtAndroid::PermissionResult::Denied){
            auto resultHash = QtAndroid::requestPermissionsSync(QStringList({permission}));
            if(resultHash[permission] == QtAndroid::PermissionResult::Denied)

                return false;
        }
    }

    return true;
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

#if defined (Q_OS_ANDROID)
    if(!requestAndroidPermissions())
        return -1;
#endif

    w.show();
    return a.exec();
}
