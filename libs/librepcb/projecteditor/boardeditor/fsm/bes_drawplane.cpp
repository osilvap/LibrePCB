/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bes_drawplane.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include "../boardeditor.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BES_DrawPlane::BES_DrawPlane(BoardEditor& editor, Ui::BoardEditor& editorUi,
                             GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mSubState(SubState_Idle), mCurrentLayerName(GraphicsLayer::sTopCopper),
    // command toolbar actions / widgets:
    mLayerLabel(nullptr), mLayerComboBox(nullptr)
{
}

BES_DrawPlane::~BES_DrawPlane()
{
    Q_ASSERT(mSubState == SubState_Idle);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_DrawPlane::process(BEE_Base* event) noexcept
{
    switch (mSubState) {
        //case SubState_Idle: return processSubStateIdle(event);
        //case SubState_PositioningNetPoint: return processSubStatePositioning(event);
        default: /*Q_ASSERT(false);*/ return PassToParentState;
    }
}

bool BES_DrawPlane::entry(BEE_Base* event) noexcept
{
    Q_UNUSED(event);
    Q_ASSERT(mSubState == SubState_Idle);

    // clear board selection because selection does not make sense in this state
    if (mEditor.getActiveBoard()) mEditor.getActiveBoard()->clearSelection();

    // add the "Layer:" label to the toolbar
    mLayerLabel = new QLabel(tr("Layer:"));
    mLayerLabel->setIndent(10);
    mEditorUi.commandToolbar->addWidget(mLayerLabel);

    // add the layers combobox to the toolbar
    mLayerComboBox = new GraphicsLayerComboBox();
    if (mEditor.getActiveBoard()) {
        QList<GraphicsLayer*> layers;
        foreach (const auto& layer, mEditor.getActiveBoard()->getLayerStack().getAllLayers()) {
            if (layer->isCopperLayer() && layer->isEnabled()) { layers.append(layer); }
        }
        mLayerComboBox->setLayers(layers);
    }
    mLayerComboBox->setCurrentLayer(mCurrentLayerName);
    mEditorUi.commandToolbar->addWidget(mLayerComboBox);
    connect(mLayerComboBox, &GraphicsLayerComboBox::currentLayerChanged,
            this, &BES_DrawPlane::layerComboBoxLayerChanged);

    // change the cursor
    mEditorGraphicsView.setCursor(Qt::CrossCursor);

    return true;
}

bool BES_DrawPlane::exit(BEE_Base* event) noexcept
{
    Q_UNUSED(event);

    // abort the currently active command
    //if (mSubState != SubState_Idle)
    //    abortPositioning(true);

    // Remove actions / widgets from the "command" toolbar
    delete mLayerComboBox;          mLayerComboBox = nullptr;
    delete mLayerLabel;             mLayerLabel = nullptr;
    qDeleteAll(mActionSeparators);  mActionSeparators.clear();

    // change the cursor
    mEditorGraphicsView.setCursor(Qt::ArrowCursor);

    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BES_DrawPlane::layerComboBoxLayerChanged(const QString& name) noexcept
{
    mCurrentLayerName = name;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
