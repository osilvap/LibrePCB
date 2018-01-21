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

#ifndef LIBREPCB_STROKEFONT_H
#define LIBREPCB_STROKEFONT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "../units/all_length_units.h"
#include "../fileio/filepath.h"
#include "../geometry/polygon.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace fontobene {
class Vertex;
class Font;
class GlyphListAccessor;
}

namespace librepcb {

/*****************************************************************************************
 *  Class StrokeFont
 ****************************************************************************************/

/**
 * @brief The StrokeFont class
 */
class StrokeFont final
{
        Q_DECLARE_TR_FUNCTIONS(StrokeFont)

    public:
        // Constructors / Destructor
        StrokeFont(const FilePath& fontFilePath);
        StrokeFont(const StrokeFont& other) noexcept;
        ~StrokeFont() noexcept;

        // General Methods
        QList<Polygon> stroke(const QString& text, const Length& size) const noexcept;
        QList<Polygon> stroke(const QChar& glyph, const Length& size) const noexcept;

        // Operator Overloadings
        StrokeFont& operator=(const StrokeFont& rhs) noexcept;


    private:
        static Point vertex2point(const fontobene::Vertex& v, const Length& size) noexcept;
        static Angle vertex2angle(const fontobene::Vertex& v) noexcept;


    private: // Data
        QScopedPointer<fontobene::Font> mFont;
        QScopedPointer<fontobene::GlyphListAccessor> mGlyphListAccessor;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_STROKEFONT_H
