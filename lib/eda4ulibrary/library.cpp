/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <QtSql>

#include "library.h"
#include <eda4ucommon/exceptions.h>
#include <eda4ucommon/fileio/filepath.h>
#include "../workspace/workspace.h"


namespace library{

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Library::Library(const FilePath& libPath) throw (Exception):
    QObject(0),
    mLibPath(libPath),
    mLibFilePath(libPath.getPathTo("lib.db"))
{
    //Select and open sqlite library 'lib.db'
    mLibDatabase = QSqlDatabase::addDatabase("QSQLITE", mLibFilePath.toNative());
    mLibDatabase.setDatabaseName(mLibFilePath.toNative());

    //Check if database is valid
    if ( ! mLibDatabase.isValid())
    {
        throw RuntimeError(__FILE__, __LINE__, mLibFilePath.toStr(),
            QString(tr("Invalid library file: \"%1\"")).arg(mLibFilePath.toNative()));
    }

    if ( ! mLibDatabase.open())
    {
        throw RuntimeError(__FILE__, __LINE__, mLibFilePath.toStr(),
            QString(tr("Could not open library file: \"%1\"")).arg(mLibFilePath.toNative()));
    }


}

Library::~Library()
{
    mLibDatabase.close();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library