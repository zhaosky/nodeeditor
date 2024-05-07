#include "DefaultNodePainter.hpp"

#include <cmath>

#include <QtCore/QMargins>

#include "AbstractGraphModel.hpp"
#include "AbstractNodeGeometry.hpp"
#include "BasicGraphicsScene.hpp"
#include "ConnectionGraphicsObject.hpp"
#include "ConnectionIdUtils.hpp"
#include "NodeGraphicsObject.hpp"
#include "NodeState.hpp"
#include "StyleCollection.hpp"

namespace QtNodes {

void DefaultNodePainter::paint(QPainter *painter, NodeGraphicsObject &ngo) const
{
    // TODO?
    //AbstractNodeGeometry & geometry = ngo.nodeScene()->nodeGeometry();
    //geometry.recomputeSizeIfFontChanged(painter->font());

    drawNodeRect(painter, ngo);

    drawCaptionRect(painter,ngo);

    drawConnectionPoints(painter, ngo);

    drawFilledConnectionPoints(painter, ngo);

    drawNodeCaption(painter, ngo);

    drawShowTime(painter,ngo);
    drawNodeIcon(painter,ngo);
    drawOperationIcon(painter,ngo);
    drawOperationResult(painter,ngo);
    //drawEntryLabels(painter, ngo);

    drawResizeRect(painter, ngo);
}

void DefaultNodePainter::drawCaptionRect(QPainter *painter, NodeGraphicsObject &ngo) const 
{
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();

    QSize size = geometry.size(ngo.nodeId());

    QRect captionRect(0+1,DEFAULT_NODE_HIGH_BEGIN+1,size.width()-2,NODE_CAPTION_HIGH-1);
    painter->fillRect(captionRect,QBrush(QColor("#253749")));

    QString strDesprition = model.nodeData(nodeId, NodeRole::Description).toString();
    QRect descriptionRect(BORDER_SPACE,DEFAULT_NODE_HIGH_BEGIN+VERTICAL_BORDER_SPACE,100,NODE_CAPTION_HIGH-2*VERTICAL_BORDER_SPACE);
    
    painter->save();
    painter->setPen(QColor(255,255,255,204));
    painter->drawText(descriptionRect,Qt::AlignLeft | Qt::AlignVCenter,strDesprition);

    painter->restore();

}

void DefaultNodePainter::drawNodeRect(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();

    NodeId const nodeId = ngo.nodeId();

    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    QSize size = geometry.size(nodeId);

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));

    NodeStyle nodeStyle(json.object());

    auto color = ngo.isSelected() ? nodeStyle.SelectedBoundaryColor : nodeStyle.NormalBoundaryColor;

    if (ngo.nodeState().hovered()) {
        QPen p(color, nodeStyle.HoveredPenWidth);
        painter->setPen(p);
    } else {
        QPen p(color, nodeStyle.PenWidth);
        painter->setPen(p);
    }

    QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(2.0, size.height()));

    gradient.setColorAt(0.0, nodeStyle.GradientColor0);
    gradient.setColorAt(0.10, nodeStyle.GradientColor1);
    gradient.setColorAt(0.90, nodeStyle.GradientColor2);
    gradient.setColorAt(1.0, nodeStyle.GradientColor3);

    painter->setBrush(gradient);

    QRectF boundary(0, DEFAULT_NODE_HIGH_BEGIN, size.width(), size.height()- DEFAULT_NODE_HIGH_BEGIN);

    double const radius = 0.0;

    painter->drawRoundedRect(boundary, radius, radius);
}

void DefaultNodePainter::drawConnectionPoints(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    auto const &connectionStyle = StyleCollection::connectionStyle();

    float diameter = nodeStyle.ConnectionPointDiameter;
    //auto reducedDiameter = diameter * 0.6;

    for (PortType portType : {PortType::Out, PortType::In}) {
        size_t const n = model
                             .nodeData(nodeId,
                                       (portType == PortType::Out) ? NodeRole::OutPortCount
                                                                   : NodeRole::InPortCount)
                             .toUInt();

        for (PortIndex portIndex = 0; portIndex < n; ++portIndex) {
            QPointF p = geometry.portPosition(nodeId, portType, portIndex);

            auto const &dataType = model.portData(nodeId, portType, portIndex, PortRole::DataType)
                                       .value<NodeDataType>();

            double r = 1.0;

            NodeState const &state = ngo.nodeState();

            if (auto const *cgo = state.connectionForReaction()) {
                PortType requiredPort = cgo->connectionState().requiredPort();

                if (requiredPort == portType) {
                    ConnectionId possibleConnectionId = makeCompleteConnectionId(cgo->connectionId(),
                                                                                 nodeId,
                                                                                 portIndex);

                    bool const possible = model.connectionPossible(possibleConnectionId);

                    auto cp = cgo->sceneTransform().map(cgo->endPoint(requiredPort));
                    cp = ngo.sceneTransform().inverted().map(cp);

                    auto diff = cp - p;
                    double dist = std::sqrt(QPointF::dotProduct(diff, diff));

                    if (possible) {
                        double const thres = 40.0;
                        r = (dist < thres) ? (2.0 - dist / thres) : 1.0;
                    } else {
                        double const thres = 80.0;
                        r = (dist < thres) ? (dist / thres) : 1.0;
                    }
                }
            }

            if (connectionStyle.useDataDefinedColors()) {
                painter->setBrush(connectionStyle.normalColor(dataType.id));
            } else {
                painter->setBrush(nodeStyle.ConnectionPointColor);
            }

            // painter->drawEllipse(p, reducedDiameter * r, reducedDiameter * r);
            painter->drawPixmap(p-QPoint(10,8),QPixmap(":/imgs/node_connect.png"));
        }
    }

    if (ngo.nodeState().connectionForReaction()) {
        ngo.nodeState().resetConnectionForReaction();
    }
}

void DefaultNodePainter::drawFilledConnectionPoints(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    auto diameter = nodeStyle.ConnectionPointDiameter;

    for (PortType portType : {PortType::Out, PortType::In}) {
        size_t const n = model
                             .nodeData(nodeId,
                                       (portType == PortType::Out) ? NodeRole::OutPortCount
                                                                   : NodeRole::InPortCount)
                             .toUInt();

        for (PortIndex portIndex = 0; portIndex < n; ++portIndex) {
            QPointF p = geometry.portPosition(nodeId, portType, portIndex);

            auto const &connected = model.connections(nodeId, portType, portIndex);

            if (!connected.empty()) {
                auto const &dataType = model
                                           .portData(nodeId, portType, portIndex, PortRole::DataType)
                                           .value<NodeDataType>();

                auto const &connectionStyle = StyleCollection::connectionStyle();
                if (connectionStyle.useDataDefinedColors()) {
                    QColor const c = connectionStyle.normalColor(dataType.id);
                    painter->setPen(c);
                    painter->setBrush(c);
                } else {
                    painter->setPen(nodeStyle.FilledConnectionPointColor);
                    painter->setBrush(nodeStyle.FilledConnectionPointColor);
                }

                painter->drawEllipse(p, diameter * 0.4, diameter * 0.4);
            }
        }
    }
}

void DefaultNodePainter::drawNodeCaption(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    //AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    if (!model.nodeData(nodeId, NodeRole::CaptionVisible).toBool())
        return;

    QString const name = model.nodeData(nodeId, NodeRole::Caption).toString();

    QFont f = painter->font();
    f.setBold(true);

    QRect captionRect(42,NODE_CAPTION_HIGH+ DEFAULT_NODE_HIGH_BEGIN+BORDER_SPACE,120,CAPTION_RECT_HIGH);//geometry.captionPosition(nodeId);

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    painter->setFont(f);
    painter->setPen(nodeStyle.FontColor);
    painter->drawText(captionRect,Qt::AlignLeft| Qt::AlignVCenter, name);

    f.setBold(false);
    painter->setFont(f);
}

void DefaultNodePainter::drawEntryLabels(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    for (PortType portType : {PortType::Out, PortType::In}) {
        unsigned int n = model.nodeData<unsigned int>(nodeId,
                                                      (portType == PortType::Out)
                                                          ? NodeRole::OutPortCount
                                                          : NodeRole::InPortCount);

        for (PortIndex portIndex = 0; portIndex < n; ++portIndex) {
            auto const &connected = model.connections(nodeId, portType, portIndex);

            QPointF p = geometry.portTextPosition(nodeId, portType, portIndex);

            if (connected.empty())
                painter->setPen(nodeStyle.FontColorFaded);
            else
                painter->setPen(nodeStyle.FontColor);

            QString s;

            if (model.portData<bool>(nodeId, portType, portIndex, PortRole::CaptionVisible)) {
                s = model.portData<QString>(nodeId, portType, portIndex, PortRole::Caption);
            } else {
                auto portData = model.portData(nodeId, portType, portIndex, PortRole::DataType);

                s = portData.value<NodeDataType>().name;
            }

            painter->drawText(p, s);
        }
    }
}

void DefaultNodePainter::drawResizeRect(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    if (model.nodeFlags(nodeId) & NodeFlag::Resizable) {
        painter->setBrush(Qt::gray);

        painter->drawEllipse(geometry.resizeHandleRect(nodeId));
    }
}
//showNode exec Time
void DefaultNodePainter::drawShowTime(QPainter *painter, NodeGraphicsObject &ngo) const 
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();

    int nTime = model.nodeData(nodeId, NodeRole::Time).toInt();
    if (nTime == 0)
        return;

    QString strTime = QString("time:%1ms").arg(nTime);
    QRect timeRect(BORDER_SPACE,NODE_CAPTION_HIGH+ DEFAULT_NODE_HIGH_BEGIN+2*BORDER_SPACE + CAPTION_RECT_HIGH,100,BORDER_SPACE);

    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    painter->save();
    painter->setPen(QColor("#A7A7A7"));
    painter->drawText(timeRect,Qt::AlignLeft| Qt::AlignVCenter, strTime);
    painter->restore();
}

void DefaultNodePainter::drawNodeIcon(QPainter *painter, NodeGraphicsObject &ngo) const 
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();

    QString strIcon = model.nodeData(nodeId, NodeRole::Icon).toString();
    if (strIcon.isEmpty())
        return;

    QRectF iconRect(BORDER_SPACE,NODE_CAPTION_HIGH+ DEFAULT_NODE_HIGH_BEGIN+BORDER_SPACE,16,16);
    QRectF iconSource(0.0,0.0,16.0,16.0);

    QPixmap pixmap = QPixmap(strIcon).scaled(QSize(16,16),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    
    painter->drawPixmap(iconRect,pixmap,iconSource);
}

void DefaultNodePainter::drawOperationIcon(QPainter *painter, NodeGraphicsObject &ngo) const 
{
    QRectF iconSource(0.0,0.0,16.0,16.0);
    painter->drawPixmap(QRectF(ngo.GetStepNextRect()),QPixmap(":/imgs/step_next.png"),iconSource);
    painter->drawPixmap(QRectF(ngo.GetStepOverRect()),QPixmap(":/imgs/step_over.png"),iconSource);
}

void DefaultNodePainter::drawOperationResult(QPainter *painter, NodeGraphicsObject &ngo) const 
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();

    NodeResultType resultType = (NodeResultType)model.nodeData(nodeId, NodeRole::ResultValue).toInt();
    QPixmap iconPixmap;
    if (resultType == NodeResultType::ResultType_NONE)
        return;
    else if (resultType == NodeResultType::ResultType_FAILED)
    {
        iconPixmap = QPixmap(":/imgs/tip_failed.png");
    }
    else if (resultType == NodeResultType::ResultType_SUCCEED)
    {
        iconPixmap = QPixmap(":/imgs/tip_success.png");
    }
    else if (resultType == NodeResultType::ResultType_UNREACHABLE)
    {
        iconPixmap = QPixmap(":/imgs/tip_error.png");
    }
    QRectF iconSource(0.0,0.0,17.0,17.0);
    QRectF iconTarget(0.0,0.0,17.0,17.0);
    painter->drawPixmap(iconTarget,iconPixmap,iconSource);

}

} // namespace QtNodes
