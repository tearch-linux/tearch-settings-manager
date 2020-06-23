/*
 *  This file is part of Manjaro Settings Manager.
 *
 *  Roland Singer <roland@manjaro.org>
 *  Ramon Buldó <ramon@manjaro.org>
 *
 *  Manjaro Settings Manager is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Manjaro Settings Manager is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Manjaro Settings Manager.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PageKeyboard.h"
#include "ui_PageKeyboard.h"

#include <KAuth>
#include <KAuthAction>

#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QMapIterator>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtWidgets/QMessageBox>

#include <fstream>
#include <iostream>

#include <QDebug>


PageKeyboard::PageKeyboard( QWidget* parent ) :
    PageWidget( parent ),
    ui( new Ui::PageKeyboard ),
    m_keyboardModel( new KeyboardModel ),
    m_keyboardProxyModel( new QSortFilterProxyModel ),
    m_keyboardPreview( new KeyBoardPreview )
{
    ui->setupUi( this );
    setTitle( tr( "Keyboard Settings" ) );
    setIcon( QPixmap( ":/images/resources/keyboard.png" ) );
    setShowApplyButton( true );
    setName( "msm_keyboard" );

    // Keyboard preview widget
    ui->KBPreviewLayout->addWidget( m_keyboardPreview );
    m_keyboardPreview->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    // Connect signals and slots
    connect( ui->buttonRestore, &QPushButton::clicked,
             this, &PageKeyboard::buttonRestore_clicked );
    connect( ui->layoutsListView, &QListView::clicked,
             this, &PageKeyboard::setDefaultIndexToVariantListView );
    connect( ui->variantsListView, &QListView::clicked,
             this, &PageKeyboard::setKeyboardPreviewLayout );
    // Disable or enable apply button
    connect( ui->layoutsListView, &QListView::clicked,
             [=] ( )
    {
        this -> setApplyEnabled( this, true );
    } );
    connect( ui->variantsListView, &QListView::clicked,
             [=] ( )
    {
        this -> setApplyEnabled( this, true );
    } );

    m_keyboardProxyModel->setSourceModel( m_keyboardModel );
    m_keyboardProxyModel->setSortLocaleAware( true );
    m_keyboardProxyModel->setSortRole( KeyboardModel::DescriptionRole );
    m_keyboardProxyModel->sort( 0, Qt::AscendingOrder );
    ui->layoutsListView->setModel( m_keyboardProxyModel );

    // Find root layout index and set it in the layoutsRootView
    QModelIndexList layoutsRootList = m_keyboardProxyModel->match( m_keyboardProxyModel->index( 0,0 ),
                                      KeyboardModel::KeyRole,
                                      "layouts",
                                      Qt::MatchFixedString );
    if ( layoutsRootList.size() == 1 )
    {
        QModelIndex layoutsRoot = layoutsRootList.first();
        ui->layoutsListView->setRootIndex( layoutsRoot );
    }
    else
        qDebug() << "Can't find keyboard layout list";

    m_layoutsSelectionProxy = new KSelectionProxyModel( ui->layoutsListView->selectionModel(), this );
    m_layoutsSelectionProxy->setSourceModel( m_keyboardModel );
    m_layoutsSelectionProxy->setFilterBehavior( KSelectionProxyModel::ChildrenOfExactSelection );

    m_variantsSortProxy = new QSortFilterProxyModel();
    m_variantsSortProxy->setSourceModel( m_layoutsSelectionProxy );
    m_variantsSortProxy->setSortLocaleAware( true );
    m_variantsSortProxy->sort( 0, Qt::AscendingOrder );
    ui->variantsListView->setModel( m_variantsSortProxy );

    // Set root index to the model combo box
    ui->modelComboBox->setModel( m_keyboardProxyModel );
    QModelIndexList modelsRootList = m_keyboardProxyModel->match( m_keyboardProxyModel->index( 0,0 ),
                                     KeyboardModel::KeyRole,
                                     "models", Qt::MatchFixedString );
    if ( modelsRootList.size() == 1 )
    {
        QModelIndex modelsRoot = modelsRootList.first();
        ui->modelComboBox->setRootModelIndex( modelsRoot );
    }
    else
        qDebug() << "Can't find keyboard model list";

    // Setting the current values for delay and rate
    ui->sliderDelay->setValue( this->getKeyboardDelay() );
    ui->sliderRate->setValue( this->getKeyboardRate() );
    ui->label_2->setText( QString::number( this->getKeyboardDelay() ) );
    ui->label_3->setText( QString::number( this->getKeyboardRate() ) );

    // Adding signals and slots for the delay and rate slider
    connect( ui->sliderDelay, &QSlider::valueChanged,
             [=] ( int value )
    {
        ui->label_2->setText( QString::number( value ) );
        this->setApplyEnabled( this, true );
    } );
    connect( ui->sliderRate, &QSlider::valueChanged,
             [=] ( int value )
    {
        ui->label_3->setText( QString::number( value ) );
        this->setApplyEnabled( this, true );
    } );
}


PageKeyboard::~PageKeyboard()
{
    delete ui;
    delete m_keyboardModel;
    delete m_keyboardProxyModel;
    delete m_keyboardPreview;
    delete m_layoutsSelectionProxy;
    delete m_variantsSortProxy;
}


void
PageKeyboard::save()
{
    setKeyboardLayout();
    configureKeystroke();
}


void
PageKeyboard::setKeyboardLayout()
{
    QString model = ui->modelComboBox->itemData( ui->modelComboBox->currentIndex(), KeyboardModel::KeyRole ).toString();
    QString layout = ui->layoutsListView->currentIndex().data( KeyboardModel::KeyRole ).toString();
    QString variant = ui->variantsListView->currentIndex().data( KeyboardModel::KeyRole ).toString();

    if ( QString::compare( variant, "default" ) == 0 )
        variant = "";

    // Set Xorg keyboard layout
    system( QString( "setxkbmap -model \"%1\" -layout \"%2\" -variant \"%3\"" ).arg( model, layout, variant ).toUtf8() );

    QVariantMap args;
    args["model"] = model;
    args["layout"] = layout;
    args["variant"] = variant;

    KAuth::Action saveAction( QLatin1String( "org.manjaro.msm.keyboard.save" ) );
    saveAction.setHelperId( QLatin1String( "org.manjaro.msm.keyboard" ) );
    saveAction.setArguments( args );
    KAuth::ExecuteJob* job = saveAction.execute();
    if ( job->exec() )
    {
        m_currentLayout = layout;
        m_currentVariant = variant;
        m_currentModel = model;
    }
    else
    {
        QMessageBox::warning( this,
                              tr( "Error!" ),
                              QString( tr( "Failed to set keyboard layout" ) ),
                              QMessageBox::Ok, QMessageBox::Ok );
    }
}


void
PageKeyboard::configureKeystroke()
{
    int delay = ui->sliderDelay->value();
    int rate  = ui->sliderRate->value();
    char command[100];
    sprintf( command, "xset r rate %d %d", delay, rate );
    system( command );
    //time to make the changes persistant throughout the reboot
    bool added_to_xinitrc = false;
    std::ifstream filein( "~/.xinitrc" );
    std::string buffer;
    std::string new_xinitrc;
    std::string prefix = "xset r rate";
    while ( std::getline( filein, buffer ) )
    {
        //condition to check if xset prev defined
        if ( buffer.substr( 0,prefix.length() ) == prefix )
        {
            buffer = command;
            added_to_xinitrc = true;
        }
        new_xinitrc = new_xinitrc + buffer + "\n";
    }
    filein.close();
    if ( !added_to_xinitrc )
        new_xinitrc = new_xinitrc + command + "\n";
    qDebug() << new_xinitrc.c_str();
    std::ofstream fileout( "~/.xinitrc",std::ios_base::app );
    fileout.write( new_xinitrc.c_str(),new_xinitrc.length() );
    fileout.close();
}


void
PageKeyboard::load()
{
    // Default focus
    ui->layoutsListView->setFocus();

    // Detect current keyboard layout, variant and model
    if ( !m_keyboardModel->getCurrentKeyboardLayout( m_currentLayout, m_currentVariant, m_currentModel ) )
        qDebug() << "Failed to determine current keyboard layout";

    if ( m_currentLayout.isEmpty() )
        m_currentLayout = "us";
    if ( m_currentVariant.isEmpty() )
        m_currentVariant = "default";
    if ( m_currentModel.isEmpty() )
        m_currentModel = "pc105";

    setLayoutsListViewIndex( m_currentLayout );
    setVariantsListViewIndex( m_currentVariant );
    setModelComboBoxIndex( m_currentModel );

    // Disable apply button
    this -> setApplyEnabled( this, false );
}


void
PageKeyboard::setLayoutsListViewIndex( const QString& layout )
{
    QModelIndexList layoutIndexList = m_keyboardProxyModel->match( ui->layoutsListView->rootIndex().child( 0,0 ),
                                      KeyboardModel::KeyRole,
                                      layout,
                                      Qt::MatchFixedString );

    if ( layoutIndexList.size() == 1 )
    {
        QModelIndex layoutIndex = layoutIndexList.first();
        ui->layoutsListView->setCurrentIndex( layoutIndex );
    }
    else
        qDebug() << QString( "Can't find the keyboard layout %1" ).arg( layout );
}


void
PageKeyboard::setVariantsListViewIndex( const QString& variant )
{
    QAbstractItemModel* model = ui->variantsListView->model();
    QModelIndexList variantDefaultList = model->match( model->index( 0,0 ),
                                         KeyboardModel::KeyRole,
                                         variant,
                                         Qt::MatchFixedString );
    if ( variantDefaultList.size() == 1 )
    {
        QModelIndex variantDefault = variantDefaultList.first();
        ui->variantsListView->setCurrentIndex( variantDefault );
        // Emit clicked(), to update keyboardPreview
        emit( ui->variantsListView->clicked( variantDefault ) );
    }
    else
        qDebug() << QString( "Can't find the keyboard variant %1" ).arg( variant );
}


void
PageKeyboard::setModelComboBoxIndex( const QString& model )
{
    QModelIndexList modelIndexList = m_keyboardProxyModel->match( ui->modelComboBox->rootModelIndex().child( 0,0 ),
                                     KeyboardModel::KeyRole,
                                     model,
                                     Qt::MatchFixedString );
    if ( modelIndexList.size() == 1 )
    {
        QModelIndex modelIndex = modelIndexList.first();
        ui->modelComboBox->setCurrentIndex( modelIndex.row() );
    }
    else
        qDebug() << QString( "Can't find the keyboard model %1" ).arg( model );
}


void
PageKeyboard::buttonRestore_clicked()
{
    setLayoutsListViewIndex( m_currentLayout );
    setVariantsListViewIndex( m_currentVariant );
    setModelComboBoxIndex( m_currentModel );

    this -> setApplyEnabled( this, false );
}


void
PageKeyboard::setDefaultIndexToVariantListView( const QModelIndex& index )
{
    if ( index.isValid() )
        setVariantsListViewIndex( "default" );
}


void
PageKeyboard::setKeyboardPreviewLayout( const QModelIndex& index )
{
    if ( index.isValid() )
    {
        QString layout = ui->layoutsListView->currentIndex().data( KeyboardModel::KeyRole ).toString();
        QString variant = ui->variantsListView->currentIndex().data( KeyboardModel::KeyRole ).toString();
        m_keyboardPreview->setLayout( layout );
        m_keyboardPreview->setVariant( variant );
    }
}


int
PageKeyboard::getKeyboardDelay()
{
    FILE* file = popen( "xset q | grep rate","r" );
    int delay,rate;
    fscanf( file,"%*[^0123456789]%d%*[^0123456789]%d",&delay,&rate );
    pclose( file );
    return delay;
}

int
PageKeyboard::getKeyboardRate()
{
    FILE* file = popen( "xset q | grep rate","r" );
    int delay,rate;
    fscanf( file,"%*[^0123456789]%d%*[^0123456789]%d",&delay,&rate );
    pclose( file );
    return rate;
}
