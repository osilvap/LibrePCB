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

#ifndef LIBREPCB_PROJECT_BOARDPLANEFRAGMENTSBUILDER_H
#define LIBREPCB_PROJECT_BOARDPLANEFRAGMENTSBUILDER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <clipper/clipper.hpp>
#include <librepcb/common/geometry/path.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class BI_Plane;
class BI_Via;
class BI_FootprintPad;

/*****************************************************************************************
 *  Class BoardPlaneFragmentsBuilder
 ****************************************************************************************/

/**
 * @brief The BoardPlaneFragmentsBuilder class
 */
class BoardPlaneFragmentsBuilder final
{
    public:

        // Constructors / Destructor
        BoardPlaneFragmentsBuilder() = delete;
        BoardPlaneFragmentsBuilder(const BoardPlaneFragmentsBuilder& other) = delete;
        BoardPlaneFragmentsBuilder(BI_Plane& plane) noexcept;
        ~BoardPlaneFragmentsBuilder() noexcept;

        // General Methods
        QVector<Path> buildFragments() noexcept;

        // Operator Overloadings
        BoardPlaneFragmentsBuilder& operator=(const BoardPlaneFragmentsBuilder& rhs) = delete;


    private: // Methods
        void addPlaneOutline();
        void clipToBoardOutline();
        void subtractOtherObjects();
        void ensureMinimumWidth();
        void flattenResult();
        void removeOrphans();

        // Helper Methods
        ClipperLib::Path createPadCutOut(const BI_FootprintPad& pad) const noexcept;
        ClipperLib::Path createViaCutOut(const BI_Via& via) const noexcept;
        static ClipperLib::Path convertHolesToCutIns(const ClipperLib::Path& outline,
                                                     const ClipperLib::Paths& holes) noexcept;
        static ClipperLib::Paths prepareHoles(const ClipperLib::Paths& holes) noexcept;
        static ClipperLib::Path rotateCutInHole(const ClipperLib::Path& hole) noexcept;
        static int getHoleConnectionPointIndex(const ClipperLib::Path& hole) noexcept;
        static void addCutInToPath(ClipperLib::Path& outline, const ClipperLib::Path& hole) noexcept;
        static int insertConnectionPointToPath(ClipperLib::Path& path, const ClipperLib::IntPoint& p) noexcept;
        static bool calcIntersectionPos(const ClipperLib::IntPoint& p1,
                                          const ClipperLib::IntPoint& p2,
                                          const ClipperLib::cInt& x,
                                          ClipperLib::cInt& y) noexcept;

        // Conversion Methods
        static ClipperLib::Paths flattenClipperTree(const ClipperLib::PolyNode& node) noexcept;
        static QVector<Path> fromClipper(const ClipperLib::Paths& paths) noexcept;
        static Path fromClipper(const ClipperLib::Path& path) noexcept;
        static Point fromClipper(const ClipperLib::IntPoint& point) noexcept;
        static ClipperLib::Paths toClipper(const QVector<Path>& paths) noexcept;
        static ClipperLib::Path toClipper(const Path& path) noexcept;
        static ClipperLib::IntPoint toClipper(const Point& point) noexcept;


    private: // Data
        BI_Plane& mPlane;
        ClipperLib::Paths mConnectedNetSignalAreas;
        ClipperLib::Paths mResult;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BOARDPLANEFRAGMENTSBUILDER_H
