TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += cbus
qtHaveModule(quick): SUBDIRS += imports
