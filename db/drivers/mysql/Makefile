
MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

CLIENT_DRIVER=$(DBDRIVERDIR)/mysql$(EXE)
EMBEDED_DRIVER=$(DBDRIVERDIR)/mesql$(EXE)

LIBES = $(DBMIDRIVERLIB) $(DBMIBASELIB) $(SQLPLIB) $(DBSTUBSLIB) \
	 $(GISLIB) $(DATETIMELIB) 

EXTRA_CFLAGS = $(MYSQLINCPATH) $(TCLINCDIR) $(TKINCDIR)
EXTRA_INC = -I$(MODULE_TOPDIR)/lib/db/dbmi_driver

DEPENDENCIES = $(GISDEP)

DRVDOC=$(GISBASE)/docs/html/grass-mysql.html
EDRVDOC=$(GISBASE)/docs/html/grass-mesql.html

OBJS 	=  \
	create_table.o \
	cursor.o \
	describe.o \
	driver.o \
	error.o \
	execute.o \
	fetch.o \
	index.o \
	listtab.o \
	main.o \
	parse.o	\
	select.o 

CLIENT_OBJS = $(OBJS) db.o
EMBEDED_OBJS = $(OBJS) dbe.o 

CLIENT_ARCH_OBJS := $(foreach obj,$(CLIENT_OBJS),$(OBJDIR)/$(obj))
EMBEDED_ARCH_OBJS := $(foreach obj,$(EMBEDED_OBJS),$(OBJDIR)/$(obj))

ifneq ($(strip $(MYSQLDLIB)),) 
ifneq ($(strip $(CXX)),) 
    EDRIVER = $(EMBEDED_DRIVER)
endif
endif

default: $(CLIENT_DRIVER) $(EDRIVER) $(DRVDOC) $(EDRVDOC)

$(CLIENT_DRIVER): $(CLIENT_ARCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CLIENT_ARCH_OBJS) $(FMODE_OBJ) $(LIBES) \
		$(MYSQLLIBPATH) $(MYSQLLIB) $(MATHLIB) $(XDRLIB)

$(EMBEDED_DRIVER): $(EMBEDED_ARCH_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(EMBEDED_ARCH_OBJS) $(FMODE_OBJ) \
		$(MYSQLDLIB) $(LIBES) $(MATHLIB) $(XDRLIB)

$(OBJDIR)/dbe.o : db.c $(DEPENDENCIES) $(LOCAL_HEADERS)
	@test -d $(OBJDIR) || mkdir $(OBJDIR)
	$(CC) -DMESQL=1 $(CFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(EXTRA_INC) $(INC) \
		-o $@ -c db.c

$(DRVDOC): grass-mysql.html
	$(INSTALL_DATA) grass-mysql.html $(DRVDOC)

$(EDRVDOC): grass-mesql.html
	$(INSTALL_DATA) grass-mesql.html $(EDRVDOC)
