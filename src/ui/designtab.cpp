#include <QVBoxLayout>
#include <QSizePolicy>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>

#include <QDebug>

#include "src/ui/designtab.h"

DesignTab::DesignTab(QWidget* parent)
    : QWidget(parent), m_grainDialog(nullptr), m_nozzleDialog(nullptr), m_motorObject(nullptr)
{   
    SetupUI();
    connect(m_newGrainButton, SIGNAL(clicked()), this, SLOT(NewGrainButton_Clicked()));
    connect(m_nozzleSettingsButton, SIGNAL(clicked()), this, SLOT(NozzleButton_Clicked()));
    connect(m_grainsDisplay, SIGNAL(SIG_GrainPositionUpdated(int, int)),
        this, SLOT(SLOT_GrainPositionUpdated(int, int)));
    connect(m_deleteGrainButton, SIGNAL(clicked()), this, SLOT(DeleteGrainButton_Clicked()));
    connect(m_grainsDisplay, SIGNAL(cellClicked(int, int)), this, SLOT(SLOT_grainTable_cellClicked(int, int)));
    m_sim = new MotorSim;
    UpdateGraphicsScene();
}
DesignTab::~DesignTab() 
{
    delete m_sim;
}
void DesignTab::SetupUI()
{
    //controls 
    QGroupBox* frame_GrainDesign = new QGroupBox(tr("Grain Design"));
    QGroupBox* frame_Params = new QGroupBox(tr("Simulation Settings"));

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    setSizePolicy(sizePolicy);    

    m_grainsDisplay = new GrainTableWidget(this);
    //grain buttons
    m_newGrainButton = new QPushButton(tr("New Grain"));
    m_deleteGrainButton = new QPushButton(tr("Delete"));
    m_deleteGrainButton->setEnabled(false);
    QGridLayout* gLayout = new QGridLayout;    
    gLayout->addWidget(m_newGrainButton, 0, 1, 2, 2);
    gLayout->addWidget(m_deleteGrainButton, 0, 0);
    frame_GrainDesign->setLayout(gLayout);

    //nozzle and sim settings
    m_nozzleSettingsButton = new QPushButton(tr("Edit Nozzle"));
    QVBoxLayout* vLayout_2 = new QVBoxLayout;
    vLayout_2->addWidget(m_nozzleSettingsButton);
    frame_Params->setLayout(vLayout_2);

    //master layout
    QGridLayout* layout = new QGridLayout;
    layout->addWidget(m_grainsDisplay, 0, 0);
    layout->addWidget(frame_GrainDesign, 0, 1);
    layout->addWidget(frame_Params, 0, 2);

    m_motorDisplayView = new QGraphicsView;
    m_motorDisplayScene = new QGraphicsScene;
    m_motorDisplayView->setScene(m_motorDisplayScene);
    m_motorDisplayView->show();

    //takes up 1 row, and two columns
    layout->addWidget(m_motorDisplayView, 1, 0, 1, 3);
    setLayout(layout);
}

//this MUST be called AFTER we setup the basic UI layout.
void DesignTab::UpdateGraphicsScene()
{
    if (!m_motorObject)
    {
        m_motorObject = new MotorGraphicsItem(100);
        knDisplay = new QGraphicsTextItem;
        m_motorDisplayScene->addItem(m_motorObject);
    }
    if (!knDisplay)
    {
        knDisplay = new QGraphicsTextItem;        
    }
    if (m_sim->HasGrains()) m_motorObject->SetGrains(m_sim->m_Grains);   
    if (m_sim->HasNozzle())
    {
        m_motorObject->SetNozzle(m_sim->m_Nozzle);
        double kn = m_sim->CalcKn();
        qDebug() << "Kn is : " << kn;
        knDisplay->setPlainText(QString::number(kn));
        knDisplay->setPos(0, 0);
        knDisplay->show();
    } 
    
    

    m_motorObject->update();
    m_motorDisplayScene->update();
    m_motorDisplayScene->setSceneRect(m_motorObject->boundingRect());
    QRectF bounds = QRectF(m_motorObject->boundingRect().left(), m_motorObject->boundingRect().top(), 
    m_motorObject->boundingRect().width() + 50, m_motorObject->boundingRect().height() + 15);
    m_motorDisplayView->fitInView(bounds, Qt::KeepAspectRatio);

}
void DesignTab::resizeEvent(QResizeEvent* event)
{
    UpdateGraphicsScene();
}
//this allows us to mark the objects as null when they are destroyed, allowing new ones to be made later on
//NOTE: they are only deleted if delete is called directly since they do NOT have attribute WA_DeleteOnClose.
//this is because I want to be able to re-open the same dialog with the same settings easily.
void DesignTab::SLOT_NozzDialogClosed()
{
    m_nozzleDialog = nullptr;
    qDebug() << "deleted nozzle dialog!";
}
void DesignTab::SLOT_GrainDialogClosed()
{
    m_grainDialog = nullptr;
    qDebug() << "deleted grain dialog!";
}

//Recieved from the grain dialog. Updates the grain table widget,
//and send sout a signal to main window that there was a new grain added
void DesignTab::SLOT_NewGrain(OpenBurnGrain *grain)
{
    int numItems = m_grainsDisplay->rowCount();
    m_grainsDisplay->setRowCount(numItems+1);
    m_grainsDisplay->setItem(numItems, 0, new QTableWidgetItem(QString::number(grain->GetLength())));
    m_grainsDisplay->setItem(numItems, 1, new QTableWidgetItem(QString::number(grain->GetDiameter())));
    m_grainsDisplay->setItem(numItems, 2, new QTableWidgetItem(QString::number(grain->GetCoreDiameter())));    
    m_grainsDisplay->setItem(numItems, 3, new QTableWidgetItem(grain->GetPropellantType().GetPropellantName()));
    m_grainsDisplay->setItem(numItems, 4, new QTableWidgetItem(QString::number(grain->GetInhibitedFaces())));
    
    m_sim->AddGrain(grain);
    emit SIG_NewGrain(grain);    
    UpdateGraphicsScene();
}
//Recieved from the grain table widget. Update the sim!
void DesignTab::SLOT_GrainPositionUpdated(int oldPos, int newPos)
{
    //todo: update sim?
    m_sim->SwapGrains(oldPos, newPos);
    UpdateGraphicsScene();
}
void DesignTab::SLOT_NozzleUpdated(OpenBurnNozzle* nozz)
{
    if (m_sim->HasNozzle())
    {
        delete m_sim->m_Nozzle;
    }
    m_sim->SetNozzle(nozz);
    
    UpdateGraphicsScene();
}
void DesignTab::NewGrainButton_Clicked()
{
    if (!m_grainDialog) //only make one!!
    {
        m_grainDialog = new GrainDialog(nullptr, true);
        connect(m_grainDialog, SIGNAL(SIG_DIALOG_NewGrain(OpenBurnGrain*)), this, SLOT(SLOT_NewGrain(OpenBurnGrain*)));
        connect(m_grainDialog, SIGNAL(destroyed()), this, SLOT(SLOT_GrainDialogClosed()));
        
    }
    m_grainDialog->show();
    m_grainDialog->activateWindow();
    m_grainDialog->raise();
}
void DesignTab::DeleteGrainButton_Clicked()
{
    for (int i = 0; i <= m_grainsDisplay->rowCount(); i++)
    {
        if (m_grainsDisplay->isItemSelected(m_grainsDisplay->item(i, 0)))
        {
            //qDebug() << "grain # " << i << "deleted.";
            m_grainsDisplay->removeRow(i);
            m_sim->RemoveGrain(i);
            m_motorObject->RemoveGrain(i);
        }
    }
    //disable the button again since we no longer have anything selected
    m_deleteGrainButton->setEnabled(false);
    UpdateGraphicsScene();
}
void DesignTab::NozzleButton_Clicked()
{
    if (!m_nozzleDialog) //only make one!!
    {
        m_nozzleDialog = new NozzleDialog;
        connect(m_nozzleDialog, SIGNAL(SIG_NozzleChanged(OpenBurnNozzle*)), this, SLOT(SLOT_NozzleUpdated(OpenBurnNozzle*)));
        connect(m_nozzleDialog, SIGNAL(destroyed()), this, SLOT(SLOT_NozzDialogClosed()));
    }
    m_nozzleDialog->show();
    m_nozzleDialog->activateWindow();
    m_nozzleDialog->raise();
}
void DesignTab::SLOT_grainTable_cellClicked(int row, int column)
{
    m_seed_grain = m_sim->m_Grains[row];
    m_deleteGrainButton->setEnabled(true);
}