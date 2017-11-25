#include <QGroupBox>
#include <QGridLayout>
#include <QPropertyAnimation>

#include "simtab.h"
#include "src/units.h"

static const QString massFluxUnits = QObject::tr("lbs/sec/inch²"); //todo: make more mass flux units?

SimulationTab::SimulationTab(OpenBurnMotor* motor, MotorSim* sim, OpenBurnSettings* settings, QWidget* parent)
    : QWidget(parent),
    m_gfxMotor(nullptr),
    m_Motor(motor),
    m_Simulator(sim),
    m_GlobalSettings(settings),
    m_SimSettingsDialog(nullptr)
{
    SetupUI();
    m_SimSettings = new MotorSimSettings;

    connect(m_Motor, &OpenBurnMotor::DesignReady,
            this, &SimulationTab::OnDesignReady);
    connect(m_Motor, &OpenBurnMotor::DesignUpdated,
            this, &SimulationTab::OnDesignUpdated);
    connect(m_btnRunSim, &QPushButton::clicked,
            this, &SimulationTab::OnRunSimButtonClicked);
    connect(m_btnSimSettings, &QPushButton::clicked,
            this, &SimulationTab::OnSimSettingsButtonClicked);
    connect(m_SimSettings, &MotorSimSettings::SettingsChanged,
            this, &SimulationTab::OnSimSettingsChanged);
    connect(m_sldBurnTimeScrubBar, &QSlider::valueChanged,
            this, &SimulationTab::OnMotorSliceChanged);
    connect(m_btnPlayAnimation, &QPushButton::clicked,
            this, &SimulationTab::OnPlayAnimationButtonClicked);

}
SimulationTab::~SimulationTab()
{
    delete m_SimSettings;
}
void SimulationTab::SetupUI()
{
    SetupPlotter();

    m_btnRunSim = new QPushButton(tr("Run Simulation"));
    m_btnRunSim->setEnabled(false);
    m_btnSimSettings = new QPushButton(tr("Simulation Settings"));

    QGroupBox* gb_Simulate = new QGroupBox(tr("Motor Simulation"));
    QGroupBox* gb_Controls = new QGroupBox(tr("Controls"));    
    QGroupBox* gb_Results = new QGroupBox(tr("Results"));

    QGridLayout* controlsLayout = new QGridLayout;
    controlsLayout->addWidget(m_btnRunSim);
    controlsLayout->addWidget(m_btnSimSettings);
    gb_Controls->setLayout(controlsLayout);

    QGridLayout* resultsLayout = new QGridLayout;
    resultsLayout->addWidget(new QLabel(tr("Max Pc:")), 0, 0);
    resultsLayout->addWidget(m_lblMaxpressure = new QLabel, 0, 1);
    resultsLayout->addWidget(new QLabel(tr("Max Core Mass Flux:")), 1, 0);
    resultsLayout->addWidget(m_lblCoreMassFlux = new QLabel, 1, 1);
    resultsLayout->addWidget(new QLabel(tr("Burn Time:")), 2, 0);
    resultsLayout->addWidget(m_lblBurnTime = new QLabel, 2, 1);
    resultsLayout->addWidget(new QLabel(tr("Total Impulse:")), 3, 0);
    resultsLayout->addWidget(m_lblTotalImpulse = new QLabel, 3, 1);
    resultsLayout->addWidget(new QLabel(tr("Delivered Isp:")), 4, 0);
    resultsLayout->addWidget(m_lblIsp = new QLabel, 4, 1);
    resultsLayout->addWidget(new QLabel(tr("Motor Designation:")), 5, 0);
    resultsLayout->addWidget(m_lblMotorDesignation = new QLabel, 5, 1);
    gb_Results->setLayout(resultsLayout);
    
    m_MotorDisplayView = new QGraphicsView;
    m_MotorDisplayScene = new QGraphicsScene;
    m_MotorDisplayView->setScene(m_MotorDisplayScene);
    m_MotorDisplayView->show();
    m_sldBurnTimeScrubBar = new QSlider(Qt::Horizontal);
    m_btnPlayAnimation = new QPushButton(tr("Burn!"));
    UpdateGraphics();

    QGridLayout* gbGridLayout = new QGridLayout;    
    gbGridLayout->addWidget(m_Plotter, 0, 0, 2, 2);
    gbGridLayout->addWidget(gb_Controls, 0, 2, 1, 1);
    gbGridLayout->addWidget(gb_Results, 1, 2, 1, 1);
    gbGridLayout->addWidget(m_sldBurnTimeScrubBar, 2, 0, 1, 2);
    gbGridLayout->addWidget(m_btnPlayAnimation, 2, 2);
    gbGridLayout->addWidget(m_MotorDisplayView, 3, 0, 1, 3);
    gbGridLayout->setRowStretch(0, 10);
    gbGridLayout->setRowStretch(2, 1);
    gbGridLayout->setRowStretch(3, 4);

    gb_Simulate->setLayout(gbGridLayout);
    
    QHBoxLayout* masterLayout = new QHBoxLayout;
    masterLayout->addWidget(gb_Simulate);
    setLayout(masterLayout);
}
void SimulationTab::UpdateSimulation()
{
    m_Simulator->RunSim(m_SimSettings);
    UpdateResults();
}
void SimulationTab::UpdateResults()
{
    //update graphs
    UpdatePlotter();

    //update results labels
    const double totalImpulseN = OpenBurnUnits::ConvertForce(
        OpenBurnUnits::ForceUnits_T::pounds_force,
        OpenBurnUnits::ForceUnits_T::newtons,
        m_Simulator->GetTotalImpulse());
    const double avgThrustN = OpenBurnUnits::ConvertForce(
        OpenBurnUnits::ForceUnits_T::pounds_force,
        OpenBurnUnits::ForceUnits_T::newtons,
        m_Simulator->GetAvgThrust());
    const double totalImpulse = OpenBurnUnits::ConvertForce(
        OpenBurnUnits::ForceUnits_T::pounds_force,
        m_GlobalSettings->m_ForceUnits,
        m_Simulator->GetTotalImpulse());
    const double deliveredIsp = m_Simulator->GetTotalImpulse() / m_Motor->GetMotorPropellantMass(); //lb-sec / lb
    const double maxPressure = OpenBurnUnits::ConvertPressure(
        OpenBurnUnits::PressureUnits_T::psi,
        m_GlobalSettings->m_PressureUnits,
        m_Simulator->GetMaxPressure());
    const double maxMassFlux = m_Simulator->GetMaxMassFlux();

    const QString pressureUnits = OpenBurnUnits::GetPressureUnitSymbol(m_GlobalSettings->m_PressureUnits);
    const QString forceUnits = OpenBurnUnits::GetForceUnitSymbol(m_GlobalSettings->m_ForceUnits);

    m_lblMaxpressure->setText(QString::number(round(maxPressure)) + " "  + pressureUnits);
    m_lblCoreMassFlux->setText(QString::number(maxMassFlux, 'g', 3) + " " + massFluxUnits);
    m_lblBurnTime->setText(QString::number(m_Simulator->GetTotalBurnTime(), 'g', 3) + " s");
    m_lblTotalImpulse->setText(QString::number(round(totalImpulse)) +
                               " " + forceUnits + "-" + tr("sec"));
    m_lblIsp->setText(QString::number(deliveredIsp, 'f', 1) + tr(" s"));

    const QString designation(OpenBurnUtil::GetMotorClass(totalImpulseN));
    const QString thrust(QString::number(round(avgThrustN)));
    const QString percent(QString::number(OpenBurnUtil::GetMotorClassPercent(totalImpulseN), 'g', 2));
    m_lblMotorDesignation->setText(percent + "% " + designation + "-" + thrust);

    //update gfx thingies
    const int numPoints = m_Simulator->GetTotalBurnTime() / m_SimSettings->timeStep;
    m_sldBurnTimeScrubBar->setRange(0, numPoints-1);
    UpdateGraphics();
}
void SimulationTab::OnDesignReady()
{
    if (m_GlobalSettings->m_redrawOnChanges)
    {
        UpdateSimulation();
    }
    m_btnRunSim->setEnabled(true);
}
void SimulationTab::OnDesignUpdated()
{
    UpdateGraphics();
}
void SimulationTab::OnSimSettingsButtonClicked()
{
    if (m_SimSettingsDialog == nullptr)
    {
        m_SimSettingsDialog = new SimSettingsDialog(m_SimSettings);
    }
    m_SimSettingsDialog->show();
    m_SimSettingsDialog->activateWindow();
    m_SimSettingsDialog->raise();
}
void SimulationTab::OnRunSimButtonClicked()
{
    UpdateSimulation();
}
void SimulationTab::OnSimSettingsChanged()
{
    UpdateSimulation();
}
void SimulationTab::OnPlayAnimationButtonClicked()
{
    QPropertyAnimation *animation = new QPropertyAnimation(m_sldBurnTimeScrubBar,"sliderPosition");
    animation->setDuration(1000.0 * m_Simulator->GetTotalBurnTime());
    animation->setStartValue(m_sldBurnTimeScrubBar->minimum());
    animation->setEndValue(m_sldBurnTimeScrubBar->maximum());
    animation->setEasingCurve(QEasingCurve::Linear);
    animation->start();
}
void SimulationTab::SetGraphNames()
{
    const QString pressureUnits = OpenBurnUnits::GetPressureUnitSymbol(m_GlobalSettings->m_PressureUnits);
    m_Plotter->graph(0)->setName(tr("Chamber Pressure") + "(" + pressureUnits + ")");
    m_Plotter->graph(1)->setName(tr("Core Mass Flux") + "(" + massFluxUnits + ")");
}
void SimulationTab::SetupPlotter()
{
    m_Plotter = new QCustomPlot;
    m_Plotter->addGraph();
    m_Plotter->graph(0)->setPen(QPen(Qt::blue));
    m_Plotter->addGraph();
    m_Plotter->graph(1)->setPen(QPen(Qt::darkRed));
    SetGraphNames();

    // give the axes some labels:
    m_Plotter->xAxis->setLabel(tr("Time"));
    m_Plotter->yAxis->setLabel(tr("Pressure"));
    m_Plotter->yAxis->axisRect()->addAxis(QCPAxis::atRight)->setTickLabelColor(Qt::darkRed);
    QCPAxis* massFluxAxis = m_Plotter->yAxis->axisRect()->axis(QCPAxis::atRight, 1);
    massFluxAxis->setLabel("Mass Flux");
    m_Plotter->legend->setVisible(true);
    // now we move the legend from the inset layout of the axis rect into the main grid layout.
    // We create a sub layout so we can generate a small gap between the plot layout cell border
    // and the legend border:
    QCPLayoutGrid *subLayout = new QCPLayoutGrid;
    m_Plotter->plotLayout()->addElement(1, 0, subLayout);
    subLayout->setMargins(QMargins(5, 0, 5, 5));
    subLayout->addElement(0, 0, m_Plotter->legend);
    // change the fill order of the legend, so it's filled left to right in columns:
    m_Plotter->legend->setFillOrder(QCPLegend::foColumnsFirst);
    // set legend's row stretch factor very small so it ends up with minimum height:
    m_Plotter->plotLayout()->setRowStretchFactor(1, 0.001);
}
void SimulationTab::UpdatePlotter()
{
    if (m_Simulator->GetResults().empty())
    {
        return;
    }
    //prepare graphs
    const int numPoints = m_Simulator->GetTotalBurnTime() / m_SimSettings->timeStep;
    const double maxPressure = OpenBurnUnits::ConvertPressure(
        OpenBurnUnits::PressureUnits_T::psi,
        m_GlobalSettings->m_PressureUnits,
        m_Simulator->GetMaxPressure());

    const double maxPressureScale = 1.10 * maxPressure;
    const double massFluxScale = 300.0f;

    QVector<double> time(numPoints), pressure(numPoints), massflux(numPoints);
    for (int i=0; i<numPoints; ++i)
    {
        time[i] = m_Simulator->GetResults()[i]->time;
        pressure[i] = OpenBurnUnits::ConvertPressure(
            OpenBurnUnits::PressureUnits_T::psi,
            m_GlobalSettings->m_PressureUnits,
            m_Simulator->GetResults()[i]->pressure);
        massflux[i] = massFluxScale * m_Simulator->GetResults()[i]->massflux;
    }
    m_Plotter->graph(0)->setData(time, pressure);
    m_Plotter->graph(1)->setData(time, massflux);

    m_Plotter->xAxis->setRange(0.f, m_Simulator->GetTotalBurnTime() + .2f);
    m_Plotter->yAxis->setRange(0, maxPressureScale);

    QCPAxis* massFluxAxis = m_Plotter->yAxis->axisRect()->axis(QCPAxis::atRight, 1);
    massFluxAxis->setRange(0.0f, maxPressureScale / massFluxScale);

    m_Plotter->replot();
    SetGraphNames();
}
void SimulationTab::OnMotorSliceChanged(int sliceIndex)
{
    if (m_Simulator->GetResults().empty()) return;
    OpenBurnMotor* motor = m_Simulator->GetResults()[sliceIndex]->motor;
    UpdateGraphics(motor);
}
void SimulationTab::UpdateGraphics(OpenBurnMotor *motor)
{
    if (m_Simulator->GetResults().empty()) return;
    if (motor == nullptr)
    {
         motor = m_Simulator->GetResults()[m_sldBurnTimeScrubBar->value()]->motor;
    }
    if (m_gfxMotor == nullptr)
    {
        m_gfxMotor = new MotorGraphicsItem(100);
        m_MotorDisplayScene->addItem(m_gfxMotor);
        if (motor->HasGrains())
        {
            m_gfxMotor->SetGrains(motor->GetGrains());
        }
        if (motor->HasNozzle())
        {
            m_gfxMotor->SetNozzle(motor->GetNozzle());
        }
    }
    m_gfxMotor->UpdateGrains(motor->GetGrains());
    //set the motor display scene to the middle of the view plus a bit of padding on the sides
    m_MotorDisplayScene->setSceneRect(m_gfxMotor->boundingRect());
    QRectF bounds = QRectF(m_gfxMotor->boundingRect().left(), m_gfxMotor->boundingRect().top(),
        m_gfxMotor->boundingRect().width() + 50, m_gfxMotor->boundingRect().height() + 15);

    m_MotorDisplayView->fitInView(bounds, Qt::KeepAspectRatio);

    //update again just in case
    repaint();
}
void SimulationTab::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    UpdateGraphics();
}
