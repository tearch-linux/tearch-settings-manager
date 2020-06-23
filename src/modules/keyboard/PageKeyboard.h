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

#ifndef PAGEKEYBOARD_H
#define PAGEKEYBOARD_H

#include "KeyboardModel.h"
#include "KeyboardPreview.h"
#include "PageWidget.h"

#include "ApplyDialog.h"

#include <KF5/KItemModels/KSelectionProxyModel>


namespace Ui
{
class PageKeyboard;
}


class PageKeyboard : public PageWidget
{
    Q_OBJECT

public:
    explicit PageKeyboard( QWidget* parent = 0 );
    ~PageKeyboard();

    void load();
    void save();

private:
    Ui::PageKeyboard* ui;
    KeyboardModel* m_keyboardModel;
    QSortFilterProxyModel* m_keyboardProxyModel;
    KeyBoardPreview* m_keyboardPreview;
    KSelectionProxyModel* m_layoutsSelectionProxy;
    QSortFilterProxyModel* m_variantsSortProxy;

    QString m_currentLayout;
    QString m_currentVariant;
    QString m_currentModel;

    void setKeyboardLayout();
    void configureKeystroke();
    void setLayoutsListViewIndex( const QString& layout );
    void setVariantsListViewIndex( const QString& variant );
    void setModelComboBoxIndex( const QString& model );

    int getKeyboardRate();
    int getKeyboardDelay();

protected slots:
    void buttonRestore_clicked();
    void setDefaultIndexToVariantListView( const QModelIndex& index );
    void setKeyboardPreviewLayout( const QModelIndex& index );
};

#endif // PAGEKEYBOARD_H
