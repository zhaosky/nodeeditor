#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>

#include <iostream>

#include "DecimalData.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class QLabel;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class NumberDisplayDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    NumberDisplayDataModel();

    ~NumberDisplayDataModel() = default;

public:
    QString caption() const override { return QStringLiteral("Result"); }

    bool captionVisible() const override { return false; }

    QString name() const override { return QStringLiteral("Result"); }
    QString descriptions()const override {return QStringLiteral("Result");};
    
    QString icon()override {return "";};

public:
    unsigned int nPorts(PortType portType) const override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex,bool bContinueExec) override;

    QWidget *embeddedWidget() override;

    double number() const;

private:
    std::shared_ptr<DecimalData> _numberData;

    QLabel *_label;
};
