#include <QGuiApplication>
#include <QQuickView>

int main(int argc, char *argv[])
{
    qputenv("QSG_INFO", "1");
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QGuiApplication app(argc, argv);

    QQuickView view;
    view.setColor(Qt::lightGray);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1280, 720);
    view.setSource(QUrl("qrc:/main.qml"));
    view.show();

    int r = app.exec();

    return r;
}
