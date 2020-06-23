/*
 *  This file is part of Manjaro Settings Manager.
 *
 *  Roland Singer <roland@manjaro.org>
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

#include "ApplyDialog.h"
#include "ui_ApplyDialog.h"

#include <QtCore/QRegularExpression>

ApplyDialog::ApplyDialog( QWidget* parent ) :
    QDialog( parent ),
    ui( new Ui::ApplyDialog )
{
    ui->setupUi( this );

    // Setup process
    process.setProcessChannelMode( QProcess::MergedChannels );

    connect( &process, SIGNAL( finished( int ) ) ,   this, SLOT( processFinished( int ) ) );
    connect( &process, SIGNAL( readyRead() )   ,   this, SLOT( process_readyRead() ) );
    connect( ui->buttonCancel, SIGNAL( clicked() ) ,   this, SLOT( buttonCancel_clicked() ) );
    connect( ui->buttonClose, SIGNAL( clicked() )  ,   this, SLOT( close() ) );
    connect( &closeTimer, SIGNAL( timeout() )    ,   this, SLOT( closeTimer_timeout() ) );
}


ApplyDialog::~ApplyDialog()
{
    delete ui;
}


int
ApplyDialog::exec( QString cmd, QStringList arguments, QString infoText, bool skipCloseTimer )
{
    closeTimer.stop();
    ui->textEdit->clear();
    ui->buttonCancel->setEnabled( true );
    ui->buttonClose->setVisible( false );
    ui->buttonCancel->setVisible( true );
    ui->buttonClose->setText( tr( "Close" ) );
    this->skipCloseTimer = skipCloseTimer;

    if ( !infoText.isEmpty() )
        ui->textEdit->append( infoText + "\n" );

    process.start( cmd, arguments );
    return QDialog::exec();
}


bool
ApplyDialog::processSuccess()
{
    if ( process.exitStatus() == QProcess::QProcess::CrashExit )
        return false;

    return ( process.exitCode() == 0 );
}


void
ApplyDialog::buttonCancel_clicked()
{
    ui->buttonCancel->setEnabled( false );

    if ( process.state() != QProcess::NotRunning )
    {
        process.terminate();
        process.waitForFinished( 3000 );
    }

    close();
}


void
ApplyDialog::processFinished( int exitCode )
{
    ui->buttonCancel->setVisible( false );
    ui->buttonClose->setVisible( true );

    if ( exitCode == 0 )
    {
        ui->textEdit->append( "\n-> " + tr( "Process finished!" ) );

        if ( skipCloseTimer )
            close();
        else
        {
            closeSec = 3;
            ui->buttonClose->setText( tr( "Close (%1)" ).arg( QString::number( closeSec ) ) );
            closeTimer.start( 1000 );
        }
    }
    else
        ui->textEdit->append( "\n-> " + tr( "Process failed!" ) );
}


void
ApplyDialog::closeTimer_timeout()
{
    --closeSec;
    ui->buttonClose->setText( tr( "Close (%1)" ).arg( QString::number( closeSec ) ) );

    if ( closeSec <= 0 )
    {
        closeTimer.stop();
        close();
    }
}


void
ApplyDialog::process_readyRead()
{
    QString data = QString::fromUtf8( process.readAll() ).trimmed();

    if ( !data.isEmpty() )
    {
        QStringList dataList = data.split( QRegularExpression( "[\r\n]" ), QString::SkipEmptyParts );
        foreach ( auto line, dataList )
        {
            if ( line != lastMessage )
            {
                ui->textEdit->append( line.remove( QRegularExpression( "\x1b[^m]*m" ) ) );
                lastMessage = line;
            }
        }
    }

}
