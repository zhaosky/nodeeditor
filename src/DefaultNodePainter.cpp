#include "DefaultNodePainter.hpp"

#include <cmath>

#include <QFileInfo>
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
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    QJsonDocument json = QJsonDocument::fromVariant(model.nodeData(nodeId, NodeRole::Style));
    NodeStyle nodeStyle(json.object());

    drawNodeRect(painter, ngo, nodeStyle);

    drawCaptionRect(painter, ngo, nodeStyle);

    drawConnectionPoints(painter, ngo, nodeStyle);

    drawFilledConnectionPoints(painter, ngo, nodeStyle);

    drawNodeCaption(painter, ngo, nodeStyle);

    drawShowTime(painter, ngo);
    drawNodeIcon(painter, ngo);
    drawOperationIcon(painter, ngo);
    drawOperationResult(painter, ngo);
    //drawEntryLabels(painter, ngo);

    drawResizeRect(painter, ngo);
}

void DefaultNodePainter::drawCaptionRect(QPainter *painter,
                                         NodeGraphicsObject &ngo,
                                         const NodeStyle &nodeStyle) const
{
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();

    QSize size = geometry.size(ngo.nodeId());
    bool bRun = model.nodeData(nodeId, NodeRole::Running).toBool();

    QRect captionRect(0 + 1, DEFAULT_NODE_HIGH_BEGIN + 1, size.width() - 2, NODE_CAPTION_HIGH - 1);
    QColor color = QColor("#253749");
    if (bRun)
        color = QColor(nodeStyle.OperationCaptionRectColor);
    else if (ngo.nodeState().hovered())
        color = QColor(nodeStyle.HoverCaptionRectColor);
    else if (ngo.isSelected())
        color = QColor(nodeStyle.SelectedCaptionRectColor);
    else
        color = QColor(nodeStyle.NormalCaptionRectColor);

    painter->fillRect(captionRect, QBrush(color));

    QString strDesprition = model.nodeData(nodeId, NodeRole::Description).toString();
    QRect descriptionRect(BORDER_SPACE,
                          DEFAULT_NODE_HIGH_BEGIN + VERTICAL_BORDER_SPACE,
                          100,
                          NODE_CAPTION_HIGH - 2 * VERTICAL_BORDER_SPACE);

    painter->save();
    painter->setPen(QColor(255, 255, 255, 204));

    QFontMetrics metrics(painter->font());
    QString elidedText = metrics.elidedText(strDesprition, Qt::ElideRight, descriptionRect.width());
    painter->drawText(descriptionRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
}

void DefaultNodePainter::drawNodeRect(QPainter *painter,
                                      NodeGraphicsObject &ngo,
                                      const NodeStyle &nodeStyle) const
{
    AbstractGraphModel &model = ngo.graphModel();

    NodeId const nodeId = ngo.nodeId();

    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    QSize size = geometry.size(nodeId);
    bool bRun = model.nodeData(nodeId, NodeRole::Running).toBool();

    auto color = nodeStyle.NormalBoundaryColor;
    if (bRun)
        color = QColor(nodeStyle.OperationBoundaryColor);
    else if (ngo.nodeState().hovered())
        color = QColor(nodeStyle.HoverBoundaryColor);
    else if (ngo.isSelected())
        color = QColor(nodeStyle.SelectedBoundaryColor);
    else
        color = QColor(nodeStyle.NormalBoundaryColor);

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

    QRectF boundary(0,
                    DEFAULT_NODE_HIGH_BEGIN,
                    size.width(),
                    size.height() - DEFAULT_NODE_HIGH_BEGIN);

    double const radius = 0.0;

    painter->drawRoundedRect(boundary, radius, radius);
}

void DefaultNodePainter::drawConnectionPoints(QPainter *painter,
                                              NodeGraphicsObject &ngo,
                                              const NodeStyle &nodeStyle) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    auto const &connectionStyle = StyleCollection::connectionStyle();

    float diameter = nodeStyle.ConnectionPointDiameter;
    auto reducedDiameter = diameter * 0.6;

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

            painter->drawEllipse(p, reducedDiameter * r, reducedDiameter * r);
            //painter->drawPixmap(p-QPoint(10,8),QPixmap(":/imgs/node_connect.svg"));
        }
    }

    if (ngo.nodeState().connectionForReaction()) {
        ngo.nodeState().resetConnectionForReaction();
    }
}

void DefaultNodePainter::drawFilledConnectionPoints(QPainter *painter,
                                                    NodeGraphicsObject &ngo,
                                                    const NodeStyle &nodeStyle) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

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

void DefaultNodePainter::drawNodeCaption(QPainter *painter,
                                         NodeGraphicsObject &ngo,
                                         const NodeStyle &nodeStyle) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    //AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

    if (!model.nodeData(nodeId, NodeRole::CaptionVisible).toBool())
        return;

    QString const name = model.nodeData(nodeId, NodeRole::Caption).toString();

    QFont f = painter->font();
    f.setBold(true);

    QRect captionRect(42,
                      NODE_CAPTION_HIGH + DEFAULT_NODE_HIGH_BEGIN + BORDER_SPACE,
                      120,
                      CAPTION_RECT_HIGH); //geometry.captionPosition(nodeId);

    QFontMetrics metrics(f);
    QString elidedText = metrics.elidedText(name, Qt::ElideRight, captionRect.width());

    painter->setFont(f);
    painter->setPen(nodeStyle.FontColor);
    painter->drawText(captionRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    f.setBold(false);
    painter->setFont(f);
}

void DefaultNodePainter::drawEntryLabels(QPainter *painter,
                                         NodeGraphicsObject &ngo,
                                         const NodeStyle &nodeStyle) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();
    AbstractNodeGeometry &geometry = ngo.nodeScene()->nodeGeometry();

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
    QRect timeRect(BORDER_SPACE,
                   NODE_CAPTION_HIGH + DEFAULT_NODE_HIGH_BEGIN + 2 * BORDER_SPACE
                       + CAPTION_RECT_HIGH,
                   100,
                   BORDER_SPACE);

    painter->save();
    painter->setPen(QColor("#A7A7A7"));
    painter->drawText(timeRect, Qt::AlignLeft | Qt::AlignVCenter, strTime);
    painter->restore();
}

void DefaultNodePainter::drawNodeIcon(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();

    QString strIcon = model.nodeData(nodeId, NodeRole::Icon).toString();

    bool bRun = model.nodeData(nodeId, NodeRole::Running).toBool();
    int nIndex = strIcon.indexOf(".svg");
    if (nIndex > 0) {
        if (bRun)
            strIcon.insert(nIndex, "_running");
        else if (ngo.nodeState().hovered())
            strIcon.insert(nIndex, "_hover");
        else if (ngo.isSelected())
            strIcon.insert(nIndex, "_selected");
    }

    if (strIcon.isEmpty())
        return;

    QRect iconRect(BORDER_SPACE, NODE_CAPTION_HIGH + DEFAULT_NODE_HIGH_BEGIN + BORDER_SPACE, 16, 16);

    QFileInfo fileInfo(strIcon);
    if (!fileInfo.exists())
        strIcon = model.nodeData(nodeId, NodeRole::Icon).toString();

    QPixmap pixmap = RenderingSvg(strIcon);

    painter->drawPixmap(iconRect, pixmap, pixmap.rect());
}

void DefaultNodePainter::drawOperationIcon(QPainter *painter, NodeGraphicsObject &ngo) const
{
    QPixmap stepPixmap = RenderingSvg(":/imgs/step_next.svg");
    QPixmap overPixmap = RenderingSvg(":/imgs/step_over.svg");
    painter->drawPixmap(ngo.GetStepNextRect(), stepPixmap, stepPixmap.rect());
    painter->drawPixmap(ngo.GetStepOverRect(), overPixmap, overPixmap.rect());
}

void DefaultNodePainter::drawOperationResult(QPainter *painter, NodeGraphicsObject &ngo) const
{
    AbstractGraphModel &model = ngo.graphModel();
    NodeId const nodeId = ngo.nodeId();

    NodeResultType resultType = (NodeResultType) model.nodeData(nodeId, NodeRole::ResultValue)
                                    .toInt();
    QPixmap iconPixmap;
    if (resultType == NodeResultType::ResultType_NONE)
        return;
    else if (resultType == NodeResultType::ResultType_FAILED) {
        iconPixmap = RenderingSvg(":/imgs/tip_failed.svg");
    } else if (resultType == NodeResultType::ResultType_SUCCEED) {
        iconPixmap = RenderingSvg(":/imgs/tip_success.svg");
    } else if (resultType == NodeResultType::ResultType_UNREACHABLE) {
        iconPixmap = RenderingSvg(":/imgs/tip_error.svg");
    }
    QRect iconTarget(0, 0, 17, 17);
    painter->drawPixmap(iconTarget, iconPixmap, iconPixmap.rect());
}

inline QPixmap DefaultNodePainter::RenderingSvg(const QString &strPath) const
{
    QSvgRenderer renderer(strPath);
    if (!renderer.isValid())
        return QPixmap();

    QSize svgSize = renderer.defaultSize(); // 获取SVG默认大小
    svgSize *= 2;                           // 将分辨率提高2倍
    // 渲染SVG到QPixmap
    QPixmap pixmap(svgSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter);
    return pixmap;
}

} // namespace QtNodes
