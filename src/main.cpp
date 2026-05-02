#include <QApplication>
#include <QStyleFactory>
#include <QSurfaceFormat>

#include "MainWindow.h"
#include "Logger.h"
#include "Types.h"

int main(int argc, char *argv[]) {
    using namespace ve;
    
    Logger::instance().setLevel(LogLevel::Info);
    LOG_INFO("Starting Video Editor...");
    
    QApplication app(argc, argv);
    
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(45, 45, 48));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(30, 30, 32));
    palette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(45, 45, 48));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(palette);
    
    MainWindow mainWindow;
    mainWindow.resize(1600, 900);
    mainWindow.show();
    
    LOG_INFO("Video Editor started successfully");
    
    return app.exec();
}
