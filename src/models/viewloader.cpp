#include "viewloader.h"
#include "layoutreader.h"
#include <json/json.h>
#include <iostream>
#include <QLineEdit>
#include <QLabel>
#include <qwt_plot.h>

ViewLoader::ViewLoader()
{

}

void ViewLoader::loadLayout(MainWindow &mainWindow)
{
    LayoutReader layoutReader = LayoutReader();

    layoutReader.loadFile("../config/layout1.json");

    for(Json::Value widgetDesc = layoutReader.getNextWidget(); layoutReader.hasWidget(); widgetDesc = layoutReader.getNextWidget())
    {
        std::string widgetName = widgetDesc["widget"].asString();
        int xPos = widgetDesc["pos"]["x"].asInt();
        int yPos = widgetDesc["pos"]["y"].asInt();

        mainWindow.AddCustomWidget(createWidget(widgetName), xPos, yPos);

        std::cout << "name: " << widgetName
                  << " xPos: " << xPos
                  << " yPos: " << yPos
                  << std::endl;
    }
}

QWidget *ViewLoader::createWidget(std::string widgetName)
{
    if(widgetName == "LineEdit")
        return new QLineEdit();
    else if(widgetName == "Label")
        return new QLabel();
    else if(widgetName == "Plot")
        return new
}
