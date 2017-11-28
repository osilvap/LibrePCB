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

#ifndef LIBREPCB_PROJECT_EDITOR_BES_DRAWPLANE_H
#define LIBREPCB_PROJECT_EDITOR_BES_DRAWPLANE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "bes_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsLayerComboBox;

namespace project {

class BI_NetPoint;
class BI_NetLine;

namespace editor {

/*****************************************************************************************
 *  Class BES_DrawPlane
 ****************************************************************************************/

/**
 * @brief The BES_DrawPlane class
 */
class BES_DrawPlane final : public BES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit BES_DrawPlane(BoardEditor& editor, Ui::BoardEditor& editorUi,
                               GraphicsView& editorGraphicsView, UndoStack& undoStack);
        ~BES_DrawPlane();

        // General Methods
        ProcRetVal process(BEE_Base* event) noexcept override;
        bool entry(BEE_Base* event) noexcept override;
        bool exit(BEE_Base* event) noexcept override;

    private:

        // Private Types

        /// Internal FSM States (substates)
        enum SubState {
            SubState_Idle,                ///< idle state [initial state]
            SubState_PositioningVertex    ///< in this state, an undo command is active!
        };


        // Private Methods
        void layerComboBoxLayerChanged(const QString& name) noexcept;


        // General Attributes
        SubState mSubState; ///< the current substate
        QString mCurrentLayerName; ///< the current board layer name

        // Widgets for the command toolbar
        QList<QAction*> mActionSeparators;
        QLabel* mLayerLabel;
        GraphicsLayerComboBox* mLayerComboBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_EDITOR_BES_DRAWPLANE_H
