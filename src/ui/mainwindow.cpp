#include <QFileDialog>
#include <QFile>
#include "mainwindow.h"
#include "src/export.h"
#include "src/ui/dialogs/exportdialog.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ImplicitIntegerAndEnumConversion"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
	m_UndoStack = new QUndoStack();
	m_DesignMotor = std::make_unique<OpenBurnMotor>();
    m_Propellants = std::make_unique<PropellantList>();
    m_Simulator = std::make_unique<MotorSim>(m_DesignMotor.get());
    m_GlobalSettings = std::make_unique<OpenBurnSettings>();
    LoadSettings("user/settings.json");
    SetupUI();

    connect(m_Simulator.get(), &MotorSim::SimulationStarted,
            this, &MainWindow::OnSimulationStarted);
    connect(m_Simulator.get(), &MotorSim::SimulationFinished,
            this, &MainWindow::OnSimulationFinished);
    connect(m_PropellantTab.get(), &PropellantTab::PropellantsUpdated,
            this, &MainWindow::OnPropellantsUpdated);
    connect(m_GlobalSettings.get(), &OpenBurnSettings::SettingsChanged,
            this, &MainWindow::OnSettingsChanged);
    connect(m_TabWidget, &QTabWidget::currentChanged,
            this, &MainWindow::OnTabChanged);
    connect(m_DesignMotor.get(), &OpenBurnMotor::NewPropellantFound,
            this, &MainWindow::OnNewPropellantFound);
    connect(m_DesignMotor.get(), &OpenBurnMotor::DuplicatePropellantFound,
            this, &MainWindow::OnDuplicatePropellantFound);
}
MainWindow::~MainWindow()
{
}
void MainWindow::SetupUI()
{
    setWindowTitle(tr("OpenBurn"));
    setGeometry(100, 100, 800, 600);
    m_MenuBar = new QMenuBar(this);
    setMenuBar(m_MenuBar);
    m_StatusBar = new QStatusBar;
    setStatusBar(m_StatusBar);
    
    m_MenuFile = new QMenu(m_MenuBar);
    m_MenuFile->setTitle(tr("File"));
    m_MenuBar->addAction(m_MenuFile->menuAction());

    m_ActionNew = new QAction(this);
    m_ActionNew->setText(tr("New"));
    m_ActionNew->setShortcuts(QKeySequence::New);    
    connect(m_ActionNew, &QAction::triggered, this, &MainWindow::OnMenuNew);
    m_MenuFile->addAction(m_ActionNew);

    m_ActionOpen = new QAction(this);
    m_ActionOpen->setText(tr("Open"));
    m_ActionOpen->setShortcuts(QKeySequence::Open);    
    connect(m_ActionOpen, &QAction::triggered, this, &MainWindow::OnMenuOpen);
    m_MenuFile->addAction(m_ActionOpen);

    m_ActionSave = new QAction(this);
    m_ActionSave->setText(tr("Save"));
    m_ActionSave->setShortcuts(QKeySequence::Save);
    connect(m_ActionSave, &QAction::triggered, this, &MainWindow::OnMenuSave);
    m_MenuFile->addAction(m_ActionSave);

    m_ActionSaveAs = new QAction(this);
    m_ActionSaveAs->setText(tr("Save As..."));
    m_ActionSaveAs->setShortcuts(QKeySequence::SaveAs);
    connect(m_ActionSaveAs, &QAction::triggered, this, &MainWindow::OnMenuSaveAs);
    m_MenuFile->addAction(m_ActionSaveAs);

    m_MenuExport = m_MenuFile->addMenu(tr("Export"));
    m_ActionEngExport = new QAction(this);
    m_ActionEngExport->setText(tr("To .eng (RockSim, OpenRocket)"));
    m_MenuExport->addAction(m_ActionEngExport);
    connect(m_ActionEngExport, &QAction::triggered, this, &MainWindow::OnMenuEngExport);

    m_MenuFile->addSeparator();

    m_ActionQuit = new QAction(this);
    m_ActionQuit->setText(tr("Quit"));
    m_ActionQuit->setShortcuts(QKeySequence::Quit);
    connect(m_ActionQuit, &QAction::triggered, this, &MainWindow::OnMenuQuit);
    m_MenuFile->addAction(m_ActionQuit);


    m_MenuEdit = new QMenu(m_MenuBar);
    m_MenuEdit->setTitle(tr("Edit"));
    m_MenuBar->addAction(m_MenuEdit->menuAction());
	m_ActionUndo = m_UndoStack->createUndoAction(this, tr("&Undo"));
	m_ActionUndo->setShortcuts(QKeySequence::Undo);
	m_ActionRedo = m_UndoStack->createRedoAction(this, tr("&Redo"));
	m_ActionRedo->setShortcuts(QKeySequence::Redo);
	m_MenuEdit->addAction(m_ActionUndo);
	m_MenuEdit->addAction(m_ActionRedo);

    m_MenuTools = new QMenu(m_MenuBar);
    m_MenuTools->setTitle(tr("Tools"));    
    m_MenuBar->addAction(m_MenuTools->menuAction());

    m_ActionSettings = new QAction(this);
    m_ActionSettings->setText(tr("Settings"));
    m_ActionSettings->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    connect(m_ActionSettings, &QAction::triggered, this, &MainWindow::OnMenuSettings);
    m_MenuTools->addAction(m_ActionSettings);

    m_MenuHelp = new QMenu(m_MenuBar);
    m_MenuHelp->setTitle(tr("Help"));
    m_MenuBar->addAction(m_MenuHelp->menuAction());

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSizePolicy(sizePolicy);
    
    m_TabWidget = new QTabWidget(this);
    m_DesignTab = std::make_unique<DesignTab>(m_DesignMotor.get(), m_Propellants.get(), m_GlobalSettings.get(), m_UndoStack);
    m_SimTab = std::make_unique<SimulationTab>(m_DesignMotor.get(), m_Simulator.get(), m_GlobalSettings.get());
    m_PropellantTab = std::make_unique<PropellantTab>(m_Propellants.get(), m_GlobalSettings.get());
    m_TabWidget->addTab(m_DesignTab.get(), tr("Design"));
    m_TabWidget->addTab(m_SimTab.get(), tr("Simulation"));
    m_TabWidget->addTab(m_PropellantTab.get(), tr("Propellants"));
    setCentralWidget(m_TabWidget);
}
void MainWindow::ResetCurrentDesign()
{
    int currentIndex = m_TabWidget->currentIndex();
	m_DesignMotor.reset();
	m_Simulator.reset();
	m_DesignTab.reset();
	m_SimTab.reset();
	m_DesignMotor = std::make_unique<OpenBurnMotor>();
    connect(m_DesignMotor.get(), &OpenBurnMotor::NewPropellantFound,
            this, &MainWindow::OnNewPropellantFound);
    connect(m_DesignMotor.get(), &OpenBurnMotor::DuplicatePropellantFound,
            this, &MainWindow::OnDuplicatePropellantFound);

    m_Simulator = std::make_unique<MotorSim>(m_DesignMotor.get());
	m_DesignTab = std::make_unique<DesignTab>(m_DesignMotor.get(), m_Propellants.get(), m_GlobalSettings.get(), m_UndoStack);
	m_SimTab = std::make_unique<SimulationTab>(m_DesignMotor.get(), m_Simulator.get(), m_GlobalSettings.get());

	m_TabWidget->insertTab(0, m_DesignTab.get(), tr("Design"));
    m_TabWidget->insertTab(1, m_SimTab.get(), tr("Simulation"));
    m_TabWidget->setCurrentIndex(currentIndex);
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn =
            QMessageBox::question( this, "OpenBurn", tr("Are you sure?\n"),
            QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes)
    {
        event->ignore();
    }
    else
    {
        event->accept();
    }
}
void MainWindow::resizeEvent(QResizeEvent * event)
{
    QApplication::sendEvent(m_DesignTab.get(), event);
    QApplication::sendEvent(m_SimTab.get(), event);
}
void MainWindow::OnMenuNew()
{
    ResetCurrentDesign();
}
void MainWindow::OnMenuOpen()
{
    //todo: if (changes)
    ResetCurrentDesign();

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
            tr("OpenBurn File (*.obm)"));

    if (!fileName.isEmpty())
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::critical(this, tr("Error"), tr("Could not open file"));
            return;
        }
        QByteArray data = file.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(data));
        QJsonObject motor = loadDoc.object();
        m_DesignMotor->ReadJSON(motor, m_Propellants.get());
        file.close();
    }
    m_CurrentDesignFilename = fileName;
    m_DesignTab->UpdateDesign();
    m_StatusBar->showMessage(tr("Opened file ") + m_CurrentDesignFilename, 3000);
}
void MainWindow::OnMenuSave()
{
    if (!m_CurrentDesignFilename.isEmpty())
    {
        SaveFile(m_CurrentDesignFilename);
    }
    else
    {
        OnMenuSaveAs();
    }
}
void MainWindow::OnMenuSaveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QString(),
            tr("OpenBurn File (*.obm)"));
    SaveFile(fileName);
}
void MainWindow::OnMenuQuit()
{
    QMessageBox::StandardButton resBtn =
    QMessageBox::question( this, "OpenBurn", tr("Are you sure?\n"),
        QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (resBtn == QMessageBox::Yes)
    {
        QCoreApplication::quit();
    }
    else
    {
        return;
    }
}
void MainWindow::OnMenuSettings()
{
    //we don't have to worry about memory management because of WA_DeleteOnClose
    auto dialog = new GlobalSettingsDialog(m_GlobalSettings.get());
    dialog->activateWindow();
    dialog->show();
    dialog->raise();
}

void MainWindow::OnMenuEngExport()
{
    if (m_Simulator->GetResultsEmpty())
    {
        QMessageBox::critical(this, tr("Error"), tr("No simulation results!"));
        return;
    }
    EngExport engExporter(m_Simulator.get());
    //we don't have to worry about memory management because of WA_DeleteOnClose
    auto dialog = new EngExportDialog(engExporter, m_Simulator.get(), m_GlobalSettings.get());
    dialog->activateWindow();
    dialog->show();
    dialog->raise();
}
void MainWindow::SaveFile(QString fileName)
{
    if (!fileName.isEmpty())
    {
        if (!fileName.contains(".obm"))
        {
            fileName += ".obm";
        }
        m_CurrentDesignFilename = fileName;
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Error"), tr("Could not save file"));
        }
        else
        {
            QJsonObject motorObject;
            m_DesignMotor->WriteJSON(motorObject);
            QJsonDocument saveDoc(motorObject);
            file.write(saveDoc.toJson());
            file.close();
            m_StatusBar->showMessage(tr("File saved."), 3000);
        }
    }
}
bool MainWindow::LoadSettings(const QString& filename)
{
    m_SettingsFileName = filename;
    QFile file(m_SettingsFileName);
    if (file.open(QIODevice::ReadOnly))
    {
        if (file.atEnd()) //is file empty?
            return false;
        QByteArray data = file.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(data));
        QJsonObject settingsObject = loadDoc.object();
        m_GlobalSettings->ReadJSON(settingsObject);
        return true;
    }
    return false;
}
bool MainWindow::SaveSettings()
{
    QFile file(m_SettingsFileName);
    if (file.open(QIODevice::WriteOnly))
    {
        QJsonObject settingsObject;
        m_GlobalSettings->WriteJSON(settingsObject);
        QJsonDocument saveDoc(settingsObject);
        file.write(saveDoc.toJson());
        return true;
    }
    return false;
}

void MainWindow::OnSimulationStarted()
{
    m_StatusBar->showMessage(tr("Starting simulation"), 3000);
}
void MainWindow::OnSimulationFinished(bool success)
{
    if (success)
    {
        m_StatusBar->showMessage(tr("Simulation finished"), 5000);
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), tr("Simulation ERROR"));        
        m_StatusBar->showMessage(tr("Simulation ERROR!"), 5000);
    }
}
//this slot sets all the grains to a new propellant type if the propellant database happens to change
void MainWindow::OnPropellantsUpdated()
{
    for (auto prop : *m_Propellants)
    {
        for (auto grain : m_DesignMotor->GetGrains())
        {
            if (grain->GetPropellantType().GetPropellantName() == prop.GetPropellantName())
            {
                grain->SetPropellantType(prop);
            }
        }    
    }
}
void MainWindow::OnSettingsChanged()
{
    SaveSettings();
    m_StatusBar->showMessage(tr("Settings updated."), 5000);

    emit m_DesignMotor->DesignUpdated();
    m_SimTab->UpdateResults();
    m_PropellantTab->UpdateSettings();
}
void MainWindow::OnTabChanged(int index)
{
    DesignTab* designTab = qobject_cast<DesignTab*>(m_TabWidget->widget(index));
    SimulationTab* simTab = qobject_cast<SimulationTab*>(m_TabWidget->widget(index));

    //a bit of a wonky hack, force the tabs to resize
    if (designTab)
    {
        designTab->resizeEvent(new QResizeEvent(designTab->size(), designTab->size()));
    }
    if (simTab)
    {
        simTab->resizeEvent(new QResizeEvent(simTab->size(), simTab->size()));
    }
}
void MainWindow::OnNewPropellantFound(OpenBurnPropellant prop)
{
    const auto brCoef = QString::number(m_GlobalSettings->m_BurnRateUnits.ConvertFrom(
        OpenBurnUnits::BurnRateUnits_T::inches_per_second,
        prop.GetBurnRateCoef()));
    const auto cStar = QString::number(m_GlobalSettings->m_VelocityUnits.ConvertFrom(
        OpenBurnUnits::VelocityUnits_T::feet_per_second,
        prop.GetCharVelocity()));
    const auto rho = QString::number(m_GlobalSettings->m_DensityUnits.ConvertFrom(
        OpenBurnUnits::DensityUnits_T::lbs_per_in_cu,
        prop.GetDensity()));

    const auto brUnits = m_GlobalSettings->m_BurnRateUnits.GetUnitName();
    const auto cstarUnits = m_GlobalSettings->m_VelocityUnits.GetUnitName();
    const auto rhoUnits = m_GlobalSettings->m_DensityUnits.GetUnitName();
    QMessageBox::StandardButton resBtn =
        QMessageBox::question( this, "New Propellant Found",
           tr("Propellant ") + prop.GetPropellantName() + tr(" not found in database!\n\n") +
           tr("BR Coef (a): ") + brCoef + " " + brUnits + "\n" +
           tr("Br Exp (n): ") + QString::number(prop.GetBurnRateExp()) + "\n" +
           tr("Characteristic Velocity (C*): ") + cStar + " " + cstarUnits + "\n" +
           tr("Density (rho): ") + rho + " " + rhoUnits + "\n" +
           tr("Specific Heat Ratio (gamma, Cp/Cv): ") + QString::number(prop.GetSpecificHeatRatio()) + "\n\n" +
           tr("Would you like to add it to the database?\n"),
           QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (resBtn == QMessageBox::Yes)
    {
        m_PropellantTab->AddNewPropellantToDatabase(prop);
    }
}
void MainWindow::OnDuplicatePropellantFound(OpenBurnPropellant dupe, const OpenBurnPropellant &prop)
{
    const auto brUnits = m_GlobalSettings->m_BurnRateUnits.GetUnitName();
    const auto cstarUnits = m_GlobalSettings->m_VelocityUnits.GetUnitName();
    const auto rhoUnits = m_GlobalSettings->m_DensityUnits.GetUnitName();
    const auto loaded = tr("Loaded from file: ");
    const auto db = tr(" -- database: ");
    QString brCoef, brExp, cStar, rho, gmma;
    if (dupe.GetBurnRateCoef() != prop.GetBurnRateCoef())
    {
        const auto a = QString::number(m_GlobalSettings->m_BurnRateUnits.ConvertFrom(
            OpenBurnUnits::BurnRateUnits_T::inches_per_second,
            prop.GetBurnRateCoef()));
        const auto a_dupe = QString::number(m_GlobalSettings->m_BurnRateUnits.ConvertFrom(
            OpenBurnUnits::BurnRateUnits_T::inches_per_second,
            dupe.GetBurnRateCoef()));
        brCoef = tr("br Coef (a) ") + loaded + a_dupe + db + a + " " + brUnits;
    }
    if (dupe.GetBurnRateExp() != prop.GetBurnRateExp())
    {
        const auto n = QString::number(prop.GetBurnRateExp());
        const auto n_dupe = QString::number(dupe.GetBurnRateExp());
        brExp = tr("br Exp (n) ") + loaded + n_dupe + db + n;
    }
    if (dupe.GetCharVelocity() != prop.GetCharVelocity())
    {
        const auto cs = QString::number(m_GlobalSettings->m_VelocityUnits.ConvertFrom(
            OpenBurnUnits::VelocityUnits_T::feet_per_second,
            prop.GetCharVelocity()));
        const auto cs_dupe = QString::number(m_GlobalSettings->m_VelocityUnits.ConvertFrom(
            OpenBurnUnits::VelocityUnits_T::feet_per_second,
            dupe.GetCharVelocity()));
        cStar = tr("Characteristic Velocity (C*) ") + loaded + cs_dupe + db + cs + " " + cstarUnits;
    }
    if (dupe.GetDensity() != prop.GetDensity())
    {
        const auto r = QString::number(m_GlobalSettings->m_DensityUnits.ConvertFrom(
            OpenBurnUnits::DensityUnits_T::lbs_per_in_cu,
            prop.GetDensity()));
        const auto r_dupe = QString::number(m_GlobalSettings->m_DensityUnits.ConvertFrom(
            OpenBurnUnits::DensityUnits_T::lbs_per_in_cu,
            dupe.GetDensity()));
        rho = tr("Density (rho) ") + loaded + r_dupe + db + r + " " + rhoUnits;
    }
    if (dupe.GetSpecificHeatRatio() != prop.GetSpecificHeatRatio())
    {
        const auto y = QString::number(prop.GetSpecificHeatRatio());
        const auto y_dupe =  QString::number(dupe.GetSpecificHeatRatio());
        gmma = tr("Specific Heat Ratio (cp/cv, gamma) ") + loaded + y_dupe + db + y;
    }
    QMessageBox::StandardButton resBtn =
        QMessageBox::question( this, "Duplicate Propellant Found",
            tr("Propellant ") + dupe.GetPropellantName() + tr(" different from database!\n\n") +
                brCoef + "\n" +
                brExp + "\n" +
                cStar + "\n" +
                rho + "\n" +
                gmma + "\n" +
                tr("Would you like to update the database?\n"),
           QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (resBtn == QMessageBox::Yes)
    {
        auto oldProp = std::find(m_Propellants->begin(), m_Propellants->end(), prop);
        if (oldProp != m_Propellants->end())
        {
            m_Propellants->erase(oldProp);
            m_Propellants->push_back(dupe);
            m_PropellantTab->SaveDatabase();
        }
    }
}

#pragma clang diagnostic pop