#include <QtNodes/ConnectionStyle>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeData>
#include <QtNodes/NodeDelegateModelRegistry>

#include <QtGui/QScreen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QVBoxLayout>

#include <QtGui/QScreen>

#include "AdditionModel.hpp"
#include "DivisionModel.hpp"
#include "MultiplicationModel.hpp"
#include "NumberDisplayDataModel.hpp"
#include "NumberSourceDataModel.hpp"
#include "SubtractionModel.hpp"

using QtNodes::ConnectionStyle;
using QtNodes::DataFlowGraphicsScene;
using QtNodes::DataFlowGraphModel;
using QtNodes::GraphicsView;
using QtNodes::NodeDelegateModelRegistry;
using QtNodes::NodeStyle;

static std::shared_ptr<NodeDelegateModelRegistry> registerDataModels()
{
    auto ret = std::make_shared<NodeDelegateModelRegistry>();
    ret->registerModel<NumberSourceDataModel>("Sources");

    ret->registerModel<NumberDisplayDataModel>("Displays");

    ret->registerModel<AdditionModel>("Operators");

    ret->registerModel<SubtractionModel>("Operators");

    ret->registerModel<MultiplicationModel>("Operators");

    ret->registerModel<DivisionModel>("Operators");

    return ret;
}

static void setStyle()
{
    NodeStyle::setNodeStyle(
        R"(
  {
    "NodeStyle": {
      "NormalBoundaryColor": [38,87, 156],
      "SelectedBoundaryColor": [64,255,251],
     "GradientColor0": "gray",
     "GradientColor1": [80, 80, 80],
     "GradientColor2": [64, 64, 64],
     "GradientColor3": [58, 58, 58],
      "ShadowColor": [20, 20, 20],
      "FontColor": "white",
      "FontColorFaded": [100, 100, 100],
      "ConnectionPointColor": [65,120,198],
      "FilledConnectionPointColor": [65,120,198],
      "PenWidth": 1.0,
      "HoveredPenWidth": 1.0,
      "ConnectionPointDiameter": 10.0,
      "Opacity": 1.0
    }
  }
  )");

    ConnectionStyle::setConnectionStyle(
        R"(
  {
    "ConnectionStyle": {
      "ConstructionColor": [65,120,198],
      "NormalColor": [65,120,198],
      "SelectedColor": [65,120,198],
      "SelectedHaloColor": [65,120,198],
      "HoveredColor": "deepskyblue",

      "LineWidth": 1.0,
      "ConstructionLineWidth": 1.0,
      "PointDiameter": 10.0,

      "UseDataDefinedColors": false
    }
  }
  )");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    setStyle();

    std::shared_ptr<NodeDelegateModelRegistry> registry = registerDataModels();

    QWidget mainWidget;

    auto menuBar = new QMenuBar();
    QMenu *menu = menuBar->addMenu("File");

    auto saveAction = menu->addAction("Save Scene");
    saveAction->setShortcut(QKeySequence::Save);

    auto loadAction = menu->addAction("Load Scene");
    loadAction->setShortcut(QKeySequence::Open);

    QVBoxLayout *l = new QVBoxLayout(&mainWidget);

    DataFlowGraphModel dataFlowGraphModel(registry);

    l->addWidget(menuBar);
    auto scene = new DataFlowGraphicsScene(dataFlowGraphModel, &mainWidget);
    scene->setOrientation(Qt::Vertical);

    auto view = new GraphicsView(scene);
    l->addWidget(view);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    QObject::connect(saveAction, &QAction::triggered, scene, [scene, &mainWidget]() {
        if (scene->save())
            mainWidget.setWindowModified(false);
    });

    QObject::connect(loadAction, &QAction::triggered, scene, &DataFlowGraphicsScene::load);

    QObject::connect(scene, &DataFlowGraphicsScene::sceneLoaded, view, &GraphicsView::centerScene);

    QObject::connect(scene, &DataFlowGraphicsScene::modified, &mainWidget, [&mainWidget]() {
        mainWidget.setWindowModified(true);
    });

    mainWidget.setWindowTitle("[*]Data Flow: simplest calculator");
    mainWidget.resize(800, 600);
    // Center window.
    mainWidget.move(QApplication::primaryScreen()->availableGeometry().center()
                    - mainWidget.rect().center());
    mainWidget.showNormal();

    return app.exec();
}
