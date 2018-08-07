SPATH= .
OPATH= ./debug
TargetMainmyrirt = $(OPATH)/pid_pst_b1.exe

MainObjectsrt= $(OPATH)/ADPSS_PID_AI.o \
             $(OPATH)/ADPSS_PID_DI.o \
             $(OPATH)/ADPSS_PID_AO.o \
             $(OPATH)/ADPSS_PID_DO.o \
             $(OPATH)/ADPSS_PID.o \
             $(OPATH)/rtftoc_intel64.o \
             $(OPATH)/ADPSS_PID_ITF.o \
             $(OPATH)/PstPIDItf_rt64.o \
             $(OPATH)/yjdpsrt.o

include /opt/virt/rt/rt.mk
MPIRTPATH=/opt/mpigm
PATHRTNET=/opt/virt/rtnet

CC = $(USER_CC) 
CPP = $(USER_CXX) 
#CCflag = $(USER_CFLAGS) -c -g -Iinclude -I$(MPIRTPATH)/include -I$(PATHRTNET)/include -I$(PATHRTNET)/include/rtnet -DERT=1
#CXXflag = $(USER_CXXFLAGS) -c -g -Iinclude -I$(MPIRTPATH)/include -I$(PATHRTNET)/include -I$(PATHRTNET)/include/rtnet -DERT=1
CCflag = $(USER_CFLAGS) -c -g -Iinclude -I$(MPIRTPATH)/include -I$(PATHRTNET)/include -I$(PATHRTNET)/include/rtnet -DERT=1 -DZX_DBG2
CXXflag = $(USER_CXXFLAGS) -c -g -Iinclude -I$(MPIRTPATH)/include -I$(PATHRTNET)/include -I$(PATHRTNET)/include/rtnet -DERT=1 -DZX_DBG2
Lflag = -L$(RT_LIBS_DIR) -lpsrt -L$(MPIRTPATH)/lib -lmpich -L/opt/gm/lib64 -lgm -L$(PATHRRTNET) -lrtnet -static

#Lflag = -L$(MPIRTPATH)/lib -lmpich -L/opt/gm/lib -lgm -L/usr/lib -L/usr/lib/gcc-lib/i386-redhat-linux/3.2.3 -static

MPICCmyrirt = $(MPIRTPATH)/bin/mpicxx

ALL : $(TargetMainmyrirt)

$(TargetMainmyrirt) : $(MainObjectsrt)
	$(MPICCmyrirt)  $(MainObjectsrt) $(Lflag)  -o $(TargetMainmyrirt)
	cp -f $(TargetMainmyrirt) /export/run

$(OPATH)/%.o:$(SPATH)/%.c
	$(CC) $(CCflag) $< -o $@
$(OPATH)/%.o:$(SPATH)/%.cpp
	$(CPP) $(CXXflag) $< -o $@

clean :
	rm -f $(TargetMainmyrirt) $(MainObjectsrt)
