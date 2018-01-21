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
#include <QtConcurrent/QtConcurrent>
#include "strokefont.h"
#include "../fileio/fileutils.h"
#include "../geometry/polygon.h"
#include <fontobene/fontobene.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

StrokeFont::StrokeFont(const FilePath& fontFilePath)
{
    try { // wrap std::exception from fontobene
        mFont.reset(new fontobene::Font());
        mFont->loadFromFile(fontFilePath.toStr());
        mGlyphListAccessor.reset(new fontobene::GlyphListAccessor(mFont->glyphs));
    } catch (const std::exception& e) {
        throw RuntimeError(__FILE__, __LINE__, QString(tr("Error while loading '%1': %2"))
                                               .arg(fontFilePath.toNative()).arg(e.what()));
    }
}

StrokeFont::StrokeFont(const StrokeFont& other) noexcept :
    mFont(new fontobene::Font(*other.mFont))
{
}

StrokeFont::~StrokeFont() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QList<Polygon> StrokeFont::stroke(const QString& text, const Length& size) const noexcept
{
    QList<Polygon> polygons;
    Length x = 0;
    foreach (const QChar& c, text) {
        QPainterPath path;
        foreach (const Polygon& p, stroke(c, size)) {
            path.addPath(p.toQPainterPathPx());
            polygons.append(p.translated(Point(x, 0)));
        }
        x += Length::fromPx(path.boundingRect().right()) + (size * 3) / 10;
    }
    return polygons;
}

QList<Polygon> StrokeFont::stroke(const QChar& glyph, const Length& size) const noexcept
{
    bool ok = false;
    QVector<fontobene::Polyline> polylines = mGlyphListAccessor->getAllPolylinesOfGlyph(glyph.unicode(), &ok);

    QList<Polygon> polygons;
    foreach (const fontobene::Polyline& l, polylines) {
        if (l.isEmpty()) continue;
        Polygon polygon(QString(), Length(0), false, false, vertex2point(l.first(), size));
        foreach (const fontobene::Vertex& v, l) {
            polygon.getSegments().append(
                std::make_shared<PolygonSegment>(vertex2point(v, size), vertex2angle(v)));
        }
        polygons.append(polygon);
    }
    return polygons;
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

StrokeFont& StrokeFont::operator=(const StrokeFont& rhs) noexcept
{
    mFont.reset(new fontobene::Font(*rhs.mFont));
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

Point StrokeFont::vertex2point(const fontobene::Vertex& v, const Length& size) noexcept
{
    return Point::fromMm(v.scaledX(size.toMm()), v.scaledY(size.toMm()));
}

Angle StrokeFont::vertex2angle(const fontobene::Vertex& v) noexcept
{
    return Angle::fromDeg(v.scaledBulge(180));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
