TEMPLATE = subdirs

SUBDIRS = \
    fontobene \
    hoedown \
    googletest \
    librepcb \
    parseagle \
    quazip \
    sexpresso

librepcb.depends = fontobene parseagle hoedown quazip sexpresso
