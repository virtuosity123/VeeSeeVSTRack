# statically linked plugins (note: dynamic linking won't work due to VCV Rack's architecture (global vars!))
EXTRALIBS+= $(call plugin_lib,Alikins)
EXTRALIBS+= $(call plugin_lib,AS)
EXTRALIBS+= $(call plugin_lib,AudibleInstruments)
EXTRALIBS+= $(call plugin_lib,BaconMusic)
EXTRALIBS+= $(call plugin_lib,Befaco)
EXTRALIBS+= $(call plugin_lib,Bidoo)
EXTRALIBS+= $(call plugin_lib,Bogaudio)
#EXTRALIBS+= $(call plugin_lib,BOKONTEPByteBeatMachine)  # unstable
EXTRALIBS+= $(call plugin_lib,cf)
#EXTRALIBS+= $(call plugin_lib,dBiz)  # now a DLL (13Jul2018)
EXTRALIBS+= $(call plugin_lib,DHE-Modules)
EXTRALIBS+= $(call plugin_lib,DrumKit)
EXTRALIBS+= $(call plugin_lib,ErraticInstruments)
EXTRALIBS+= $(call plugin_lib,ESeries)
EXTRALIBS+= $(call plugin_lib,FrozenWasteland)
EXTRALIBS+= $(call plugin_lib,Fundamental)
EXTRALIBS+= $(call plugin_lib,Gratrix)
EXTRALIBS+= $(call plugin_lib,HetrickCV)
EXTRALIBS+= $(call plugin_lib,huaba)
EXTRALIBS+= $(call plugin_lib,JW-Modules)
EXTRALIBS+= $(call plugin_lib,Koralfx-Modules)
EXTRALIBS+= $(call plugin_lib,LindenbergResearch)
EXTRALIBS+= $(call plugin_lib,LOGinstruments)
EXTRALIBS+= $(call plugin_lib,ML_modules)
EXTRALIBS+= $(call plugin_lib,moDllz)
EXTRALIBS+= $(call plugin_lib,modular80)
EXTRALIBS+= $(call plugin_lib,mscHack)
EXTRALIBS+= $(call plugin_lib,mtsch-plugins)
EXTRALIBS+= $(call plugin_lib,NauModular)
EXTRALIBS+= $(call plugin_lib,Ohmer)
#EXTRALIBS+= $(call plugin_lib,ParableInstruments)
EXTRALIBS+= $(call plugin_lib,Qwelk)
EXTRALIBS+= $(call plugin_lib,RJModules)
EXTRALIBS+= $(call plugin_lib,SerialRacker)
EXTRALIBS+= $(call plugin_lib,SonusModular)
EXTRALIBS+= $(call plugin_lib,Southpole-parasites)
EXTRALIBS+= $(call plugin_lib,squinkylabs-plug1)
EXTRALIBS+= $(call plugin_lib,SubmarineFree)
EXTRALIBS+= $(call plugin_lib,Template)
EXTRALIBS+= $(call plugin_lib,trowaSoft)
EXTRALIBS+= $(call plugin_lib,unless_modules)
EXTRALIBS+= $(call plugin_lib,Valley)
#EXTRALIBS+= $(call plugin_lib,VultModules)
