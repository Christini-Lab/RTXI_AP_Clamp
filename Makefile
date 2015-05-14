PLUGIN_NAME = AP_Clamp

HEADERS = AP_Clamp.h

SOURCES = AP_Clamp.cpp moc_AP_Clamp.cpp \
	include/APC_MainWindowUI.cpp include/moc_APC_MainWindowUI.cpp \
	include/APC_Protocol.cpp include/moc_APC_Protocol.cpp \
	include/APC_AddStepDialogUI.cpp include/moc_APC_AddStepDialogUI.cpp

LIBS = -lgsl -lgslcblas

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile

clean:
	rm -f $(OBJECTS)
	rm -f moc_*
	rm -f *.o
	rm -f $(PLUGIN_NAME).la
	rm -f $(PLUGIN_NAME).o
	rm -rf .libs
	rm -rf include/.libs
	rm -f include/*.o
	rm -f include/moc_*
