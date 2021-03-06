#include <QLabel>

#include "nozzledesigntypes.hpp"
#include "src/util.hpp"

OpenBurnDesignNozzle::OpenBurnDesignNozzle(QWidget* parent, OpenBurnNozzle* nozz, OpenBurnSettings* settings)
    : QWidget(parent),
      m_nozzleSeed(nozz),
      m_GlobalSettings(settings)
{
    SetupUI();
    SeedValues();

    //new function pointer syntax does not work for overloaded signals or slots >.<
    connect(m_sbThroatDia, SIGNAL(valueChanged(double)),
            this, SIGNAL(DesignUpdated()));
    connect(m_sbExitDia, SIGNAL(valueChanged(double)),
            this, SIGNAL(DesignUpdated()));
}
void OpenBurnDesignNozzle::SetupUI()
{
    m_Layout = new QGridLayout(this);
    //nozzle type
    m_cbNozzleType = new QComboBox(this);
    m_cbNozzleType->addItem(tr("Conical Nozzle"));
    QLabel* label_0 = new QLabel(tr("Nozzle Type"));
    m_Layout->addWidget(label_0, 0, 0);
    m_Layout->addWidget(m_cbNozzleType, 0, 1);

    //throat diameter
    m_sbThroatDia = new QDoubleSpinBox(this);
    QLabel* label = new QLabel(tr("Throat Diameter"), this);
    m_unitsThroatDia = new LengthUnitsComboBox(this, m_sbThroatDia);
    m_Layout->addWidget(label, 1, 0);
    m_Layout->addWidget(m_sbThroatDia, 1, 1);
    m_Layout->addWidget(m_unitsThroatDia, 1, 2);

    //exit (expansion, divergence) diameter
    m_sbExitDia = new QDoubleSpinBox(this);
    QLabel* label_2 = new QLabel(tr("Exit Diameter"), this);
    m_unitsExitDia = new LengthUnitsComboBox(this, m_sbExitDia);
    m_Layout->addWidget(label_2, 2, 0);
    m_Layout->addWidget(m_sbExitDia, 2, 1);
    m_Layout->addWidget(m_unitsExitDia, 2, 2);
    
    if (m_GlobalSettings)
    {
        m_unitsThroatDia->SetUnits(m_GlobalSettings->m_LengthUnits);
        m_unitsExitDia->SetUnits(m_GlobalSettings->m_LengthUnits);
    }
    setTabOrder(m_sbThroatDia, m_sbExitDia);

}
void OpenBurnDesignNozzle::SeedValues()
{
    if (m_nozzleSeed)
    {
        m_sbThroatDia->setValue(m_unitsThroatDia->GetCurrentUnits().ConvertFrom(
            OpenBurnUnits::LengthUnits_T::inches, 
            m_nozzleSeed->GetNozzleThroat()));
        m_sbExitDia->setValue(m_unitsExitDia->GetCurrentUnits().ConvertFrom(
            OpenBurnUnits::LengthUnits_T::inches, 
            m_nozzleSeed->GetNozzleExit()));
    }
}
double OpenBurnDesignNozzle::GetThroatDiameter()
{
    return m_unitsThroatDia->GetCurrentUnits().ConvertTo(
        OpenBurnUnits::LengthUnits_T::inches,
        m_sbThroatDia->value());
}
double OpenBurnDesignNozzle::GetExitDiameter()
{
    return m_unitsExitDia->GetCurrentUnits().ConvertTo(
        OpenBurnUnits::LengthUnits_T::inches,
        m_sbExitDia->value());
}
OpenBurnNozzle* OpenBurnDesignNozzle::GetNozzle()
{
    return m_nozzleSeed;
}
void OpenBurnDesignNozzle::AddNewControls(QWidget* widget, int row, int col)
{
    m_Layout->addWidget(widget, row+3, col);
}

ConicalNozzleDesign::ConicalNozzleDesign(QWidget* parent, ConicalNozzle* nozz, OpenBurnSettings *settings)
    : OpenBurnDesignNozzle(parent, nozz, settings)
{
    m_sbHalfAngle = new QDoubleSpinBox(this);
    m_sbHalfAngle->setDecimals(1);
    m_sbHalfAngle->setSingleStep(1);
    m_sbHalfAngle->setValue(15.f); //default to 15 degrees :)
    QLabel* label_3 = new QLabel(tr("Divergent Half Angle"), this);
    m_unitsHalfAngle = new AngleUnitsComboBox(this);
    m_unitsHalfAngle->setLayoutDirection(Qt::LeftToRight);
    if (m_GlobalSettings)
    {
        m_unitsHalfAngle->SetUnits(m_GlobalSettings->m_AngleUnits);
    }

    AddNewControls(label_3, 0, 0);
    AddNewControls(m_sbHalfAngle, 0, 1);
    AddNewControls(m_unitsHalfAngle, 0, 2);
    setTabOrder(m_sbExitDia, m_sbHalfAngle);
    SeedValues();
    connect(m_sbHalfAngle, SIGNAL(valueChanged(double)),
            this, SIGNAL(DesignUpdated()));
    
}
void ConicalNozzleDesign::SeedValues()
{
    if ( ConicalNozzle* nozz = (dynamic_cast<ConicalNozzle*>(m_nozzleSeed)))
    {
        m_sbHalfAngle->setValue(m_unitsHalfAngle->GetCurrentUnits().ConvertFrom(
            OpenBurnUnits::AngleUnits_T::degrees, 
            nozz->GetHalfAngle()));
    }
}
double ConicalNozzleDesign::GetDivergentHalfAngle()
{
    return m_unitsHalfAngle->GetCurrentUnits().ConvertTo(
        OpenBurnUnits::AngleUnits_T::degrees,
        m_sbHalfAngle->value());
}
