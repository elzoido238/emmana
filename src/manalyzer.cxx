/// EMMA MIDAS analyzer
///
/// \file: manalyzer.cxx
/// \author K.Olchanski, D. Connolly
/// \brief implementation of manalyzer.h
///

#include "manalyzer.h"
#include "midasio.h"

//////////////////////////////////////////////////////////

static bool gTrace = false;

//////////////////////////////////////////////////////////
//
// Methods of TARunInfo
//
//////////////////////////////////////////////////////////

TARunInfo::TARunInfo(int runno, const char* filename, const std::vector<std::string>& args)
{
   if (gTrace)
      printf("TARunInfo::ctor!\n");
   fRunNo = runno;
   if (filename)
      fFileName = filename;
   fOdb = NULL;
#ifdef HAVE_ROOT
   fRoot = new TARootHelper(this);
#endif
   fArgs = args;
}

TARunInfo::~TARunInfo()
{
   if (gTrace)
      printf("TARunInfo::dtor!\n");
   fRunNo = 0;
   fFileName = "(deleted)";
   if (fOdb) {
      delete fOdb;
      fOdb = NULL;
   }
#ifdef HAVE_ROOT
   if (fRoot) {
      delete fRoot;
      fRoot = NULL;
   }
#endif
}

//////////////////////////////////////////////////////////
//
// Methods of TAFlowEvent
//
//////////////////////////////////////////////////////////

TAFlowEvent::TAFlowEvent(TAFlowEvent* flow) // ctor
{
   if (gTrace)
      printf("TAFlowEvent::ctor: chain %p\n", flow);
   fNext = flow;
}

TAFlowEvent::~TAFlowEvent() // dtor
{
   if (gTrace)
      printf("TAFlowEvent::dtor: this %p, next %p\n", this, fNext);
   if (fNext)
      delete fNext;
   fNext = NULL;
}

//////////////////////////////////////////////////////////
//
// Methods of TARunObject
//
//////////////////////////////////////////////////////////

TARunObject::TARunObject(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::ctor, run %d\n", runinfo->fRunNo);
}

void TARunObject::BeginRun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::BeginRun, run %d\n", runinfo->fRunNo);
}

void TARunObject::EndRun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::EndRun, run %d\n", runinfo->fRunNo);
}

void TARunObject::NextSubrun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::NextSubrun, run %d\n", runinfo->fRunNo);
}

void TARunObject::PauseRun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::PauseRun, run %d\n", runinfo->fRunNo);
}

void TARunObject::ResumeRun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::ResumeRun, run %d\n", runinfo->fRunNo);
}

void TARunObject::PreEndRun(TARunInfo* runinfo, std::deque<TAFlowEvent*>* flow_queue)
{
   if (gTrace)
      printf("TARunObject::PreEndRun, run %d\n", runinfo->fRunNo);
}

TAFlowEvent* TARunObject::Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
{
   if (gTrace)
      printf("TARunObject::Analyze!\n");
   return flow;
}

TAFlowEvent* TARunObject::AnalyzeFlowEvent(TARunInfo* runinfo, TAFlags* flags, TAFlowEvent* flow)
{
   if (gTrace)
      printf("TARunObject::Analyze!\n");
   return flow;
}

void TARunObject::AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
{
   if (gTrace)
      printf("TARunObject::AnalyzeSpecialEvent!\n");
}

//////////////////////////////////////////////////////////
//
// Methods of TAFactory
//
//////////////////////////////////////////////////////////

void TAFactory::Init(const std::vector<std::string> &args)
{
   if (gTrace)
      printf("TAFactory::Init!\n");
}

void TAFactory::Finish()
{
   if (gTrace)
      printf("TAFactory::Finish!\n");
}


//////////////////////////////////////////////////////////
//
// Methods of TARootHelper
//
//////////////////////////////////////////////////////////


TApplication* TARootHelper::fgApp = NULL;
TDirectory*   TARootHelper::fgDir = NULL;
XmlServer*    TARootHelper::fgXmlServer = NULL;
THttpServer*  TARootHelper::fgHttpServer = NULL;

TARootHelper::TARootHelper(const TARunInfo* runinfo) // ctor
{
   if (gTrace)
      printf("TARootHelper::ctor!\n");

   char xfilename[1024];
   // char* format = "${DH}/output%05d.root";
   sprintf(xfilename, "output%05d.root", runinfo->fRunNo);

   fOutputFile = new TFile(xfilename, "RECREATE");

   assert(fOutputFile->IsOpen()); // FIXME: survive failure to open ROOT file

   fOutputFile->cd();

#ifdef XHAVE_LIBNETDIRECTORY
   NetDirectoryExport(fOutputFile, "ManalyzerOutputFile");
#endif
#ifdef HAVE_XMLSERVER
   if (fgXmlServer)
      fgXmlServer->Export(fOutputFile, "ManalyzerOutputFile");
#endif
}

TARootHelper::~TARootHelper() // dtor
{
   if (gTrace)
      printf("TARootHelper::dtor!\n");

   if (fOutputFile != NULL) {
      fOutputFile->Write();
      fOutputFile->Close();
      fOutputFile = NULL;
   }

   if (fgDir)
      fgDir->cd();
}

//////////////////////////////////////////////////////////
//
// Methods of TARegister
//
//////////////////////////////////////////////////////////

std::vector<TAFactory*> *gModules = NULL;

TARegister::TARegister(TAFactory* m)
{
   if (!gModules)
      gModules = new std::vector<TAFactory*>;
   gModules->push_back(m);
}

#if 0
static double GetTimeSec()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec + 0.000001*tv.tv_usec;
}
#endif

RunHandler::RunHandler(const std::vector<std::string>& args) { // ctor
   fRunInfo = NULL;
   fArgs = args;
}

RunHandler::~RunHandler() {//dtor
   if (fRunInfo) {
      delete fRunInfo;
      fRunInfo = NULL;
   }
}

void RunHandler::CreateRun(int run_number, const char* file_name) {
   assert(fRunInfo == NULL);
   assert(fRunRun.size() == 0);

   fRunInfo = new TARunInfo(run_number, file_name, fArgs);

   for (unsigned i=0; i<(*gModules).size(); i++)
      fRunRun.push_back((*gModules)[i]->NewRunObject(fRunInfo));
}

void RunHandler::BeginRun()
{
   assert(fRunInfo != NULL);
   assert(fRunInfo->fOdb != NULL);
   for (unsigned i=0; i<fRunRun.size(); i++)
      fRunRun[i]->BeginRun(fRunInfo);
}

void RunHandler::EndRun()
{
   assert(fRunInfo);

   std::deque<TAFlowEvent*> flow_queue;

   for (unsigned i=0; i<fRunRun.size(); i++)
      fRunRun[i]->PreEndRun(fRunInfo, &flow_queue);

   while (!flow_queue.empty()) {
      TAFlowEvent* flow = flow_queue.front();
      flow_queue.pop_front();

      int flags = 0;

      for (unsigned i=0; i<fRunRun.size(); i++) {
         flow = fRunRun[i]->AnalyzeFlowEvent(fRunInfo, &flags, flow);
         if (flags & TAFlag_SKIP)
            break;
      }

      delete flow;
   }

   for (unsigned i=0; i<fRunRun.size(); i++)
      fRunRun[i]->EndRun(fRunInfo);
}

void RunHandler::NextSubrun()
{
   assert(fRunInfo);

   for (unsigned i=0; i<fRunRun.size(); i++)
      fRunRun[i]->NextSubrun(fRunInfo);
}

void RunHandler::DeleteRun()
{
   assert(fRunInfo);

   for (unsigned i=0; i<fRunRun.size(); i++) {
      delete fRunRun[i];
      fRunRun[i] = NULL;
   }

   fRunRun.clear();
   assert(fRunRun.size() == 0);

   delete fRunInfo;
   fRunInfo = NULL;
}

void RunHandler::AnalyzeSpecialEvent(TMEvent* event)
{
   for (unsigned i=0; i<fRunRun.size(); i++)
      fRunRun[i]->AnalyzeSpecialEvent(fRunInfo, event);
}

void RunHandler::AnalyzeEvent(TMEvent* event, TAFlags* flags, TMWriterInterface *writer)
{
   assert(fRunInfo != NULL);
   assert(fRunInfo->fOdb != NULL);

   TAFlowEvent* flow = NULL;

   for (unsigned i=0; i<fRunRun.size(); i++) {
      flow = fRunRun[i]->Analyze(fRunInfo, event, flags, flow);
      if (*flags & TAFlag_SKIP)
         break;
   }

   if (flow && !(*flags & TAFlag_SKIP)) {
      for (unsigned i=0; i<fRunRun.size(); i++) {
         flow = fRunRun[i]->AnalyzeFlowEvent(fRunInfo, flags, flow);
         if (*flags & TAFlag_SKIP)
            break;
      }
   }

   if (*flags & TAFlag_WRITE)
      if (writer)
         TMWriteEvent(writer, event);

   if (flow)
      delete flow;
}



OnlineHandler::OnlineHandler(int num_analyze, TMWriterInterface* writer, const std::vector<std::string>& args) // ctor
   : fRun(args)
{
   fQuit = false;
   fNumAnalyze = num_analyze;
   fWriter = writer;
}

OnlineHandler::~OnlineHandler() // dtor
{
   fWriter = NULL;
}

void OnlineHandler::StartRun(int run_number)
{
   fRun.CreateRun(run_number, NULL);
   fRun.fRunInfo->fOdb = TMidasOnline::instance();
   fRun.BeginRun();
}

void OnlineHandler::Transition(int transition, int run_number, int transition_time)
{
   //printf("OnlineHandler::Transtion: transition %d, run %d, time %d\n", transition, run_number, transition_time);

   if (transition == TR_START) {
      if (fRun.fRunInfo) {
         fRun.EndRun();
         fRun.fRunInfo->fOdb = NULL;
         fRun.DeleteRun();
      }
      assert(fRun.fRunInfo == NULL);

      StartRun(run_number);
      printf("Begin run: %d\n", run_number);
   } else if (transition == TR_STOP) {
      fRun.EndRun();
      fRun.fRunInfo->fOdb = NULL;
      fRun.DeleteRun();
      printf("End of run %d\n", run_number);
   }
}

void OnlineHandler::Event(const void* data, int data_size)
{
   //printf("OnlineHandler::Event: ptr %p, size %d\n", data, data_size);

   if (!fRun.fRunInfo) {
      StartRun(0); // start fake run for events outside of a run
   }

   TMEvent* event = new TMEvent(data, data_size);

   TAFlags flags = 0;

   fRun.AnalyzeEvent(event, &flags, fWriter);

   if (flags & TAFlag_QUIT)
      fQuit = true;

   if (fNumAnalyze > 0) {
      fNumAnalyze--;
      if (fNumAnalyze == 0)
         fQuit = true;
   }

   if (event) {
      delete event;
      event = NULL;
   }
}

static int ProcessMidasOnline(const std::vector<std::string>& args, const char* hostname, const char* exptname, int num_analyze, TMWriterInterface* writer)
{
   TMidasOnline *midas = TMidasOnline::instance();

   int err = midas->connect(hostname, exptname, "rootana");
   if (err != 0) {
      fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
      return -1;
   }

   OnlineHandler* h = new OnlineHandler(num_analyze, writer, args);

   midas->RegisterHandler(h);
   midas->registerTransitions();

   /* reqister event requests */

   midas->eventRequest("SYSTEM",-1,-1,(1<<1));

   int run_number = midas->odbReadInt("/runinfo/Run number");
   int run_state  = midas->odbReadInt("/runinfo/State");

   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Init(args);

   if ((run_state == STATE_RUNNING)||(run_state == STATE_PAUSED)) {
      h->StartRun(run_number);
   }

   while (!h->fQuit) {
#ifdef HAVE_THTTP_SERVER
      if (TARootHelper::fgHttpServer) {
         TARootHelper::fgHttpServer->ProcessRequests();
      }
#endif
#ifdef HAVE_ROOT
      if (TARootHelper::fgApp) {
         gSystem->DispatchOneEvent(kTRUE);
      }
#endif
      if (!TMidasOnline::instance()->poll(10))
         break;
   }

   if (h->fRun.fRunInfo) {
      h->fRun.EndRun();
      h->fRun.fRunInfo->fOdb = NULL;
      h->fRun.DeleteRun();
   }

   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Finish();

   /* disconnect from experiment */
   midas->disconnect();

   return 0;
}

static int ProcessMidasFiles(const std::vector<std::string>& files, const std::vector<std::string>& args, int num_skip, int num_analyze, TMWriterInterface* writer)
{
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Init(args);

   RunHandler run(args);

   bool done = false;

   for (unsigned i=0; i<files.size(); i++) {
      std::string filename = files[i];

      TMReaderInterface *reader = TMNewReader(filename.c_str());

      if (reader->fError) {
         printf("Could not open \"%s\", error: %s\n", filename.c_str(), reader->fErrorString.c_str());
         delete reader;
         continue;
      }

      while (1) {
         TMEvent* event = TMReadEvent(reader);

         if (!event) // EOF
            break;

         if (event->error) {
            delete event;
            break;
         }

         if (event->event_id == 0x8000) // begin of run event
            {
               int runno = event->serial_number;

               if (run.fRunInfo) {
                  if (run.fRunInfo->fRunNo == runno) {
                     // next subrun file, nothing to do
                     run.fRunInfo->fFileName = filename;
                     run.NextSubrun();
                  } else {
                     // file with a different run number
                     run.EndRun();
                     run.DeleteRun();
                  }
               }

               if (!run.fRunInfo) {
                  run.CreateRun(runno, filename.c_str());
#ifdef HAVE_ROOT_XML
                  run.fRunInfo->fOdb = new XmlOdb(event->GetEventData(), event->data_size);
#else
                  run.fRunInfo->fOdb = new EmptyOdb();
#endif
                  run.BeginRun();
               }

               assert(run.fRunInfo);

               run.AnalyzeSpecialEvent(event);

               if (writer)
                  TMWriteEvent(writer, event);
            }
         else if (event->event_id == 0x8001) // end of run event
            {
               //int runno = event->serial_number;
               run.AnalyzeSpecialEvent(event);
               if (writer)
                  TMWriteEvent(writer, event);

               if (run.fRunInfo->fOdb) {
                  delete run.fRunInfo->fOdb;
                  run.fRunInfo->fOdb = NULL;
               }

#ifdef HAVE_ROOT_XML
               run.fRunInfo->fOdb = new XmlOdb(event->GetEventData(), event->data_size);
#else
               run.fRunInfo->fOdb = new EmptyOdb();
#endif
            }
         else if (event->event_id == 0x8002) // message event
            {
               run.AnalyzeSpecialEvent(event);
               if (writer)
                  TMWriteEvent(writer, event);
            }
         else
            {
               if (!run.fRunInfo) {
                  // create a fake begin of run
                  run.CreateRun(0, filename.c_str());
                  run.fRunInfo->fOdb = new EmptyOdb();
                  run.BeginRun();
               }

               if (num_skip > 0) {
                  num_skip--;
               } else {
                  TAFlags flags = 0;

                  run.AnalyzeEvent(event, &flags, writer);

                  if (flags & TAFlag_QUIT)
                     done = true;

                  if (num_analyze > 0) {
                     num_analyze--;
                     if (num_analyze == 0)
                        done = true;
                  }
               }
            }

         delete event;

         if (done)
            break;

#ifdef HAVE_ROOT
         if (TARootHelper::fgApp) {
            gSystem->DispatchOneEvent(kTRUE);
         }
#endif
      }

      reader->Close();
      delete reader;

      if (done)
         break;
   }

   if (run.fRunInfo) {
      run.EndRun();
      run.DeleteRun();
   }

   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Finish();

   return 0;
}

static bool gEnableShowMem = false;

#if 0
static int ShowMem(const char* label)
{
   if (!gEnableShowMem)
      return 0;

   FILE* fp = fopen("/proc/self/statm","r");
   if (!fp)
      return 0;

   int mem = 0;
   fscanf(fp,"%d",&mem);
   fclose(fp);

   if (label)
      printf("memory at %s is %d\n", label, mem);

   return mem;
}
#endif

// ==================== EventDumpModule Methods ==================== //

EventDumpModule::EventDumpModule(TARunInfo* runinfo)
   : TARunObject(runinfo)
{
   if (gTrace)
      printf("EventDumpModule::ctor, run %d\n", runinfo->fRunNo);
}

EventDumpModule::~EventDumpModule()
{
   if (gTrace)
      printf("EventDumpModule::dtor!\n");
}

void EventDumpModule::BeginRun(TARunInfo* runinfo)
{
   printf("EventDumpModule::BeginRun, run %d\n", runinfo->fRunNo);
}

void EventDumpModule::EndRun(TARunInfo* runinfo)
{
   printf("EventDumpModule::EndRun, run %d\n", runinfo->fRunNo);
}

void EventDumpModule::NextSubrun(TARunInfo* runinfo)
{
   printf("EventDumpModule::NextSubrun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
}

void EventDumpModule::PauseRun(TARunInfo* runinfo)
{
   printf("EventDumpModule::PauseRun, run %d\n", runinfo->fRunNo);
}

void EventDumpModule::ResumeRun(TARunInfo* runinfo)
{
   printf("EventDumpModule::ResumeRun, run %d\n", runinfo->fRunNo);
}

TAFlowEvent* EventDumpModule::Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
{
   printf("EventDumpModule::Analyze, run %d, ", runinfo->fRunNo);
   event->FindAllBanks();
   std::string h = event->HeaderToString();
   std::string b = event->BankListToString();
   printf("%s: %s\n", h.c_str(), b.c_str());
   return flow;
}

void EventDumpModule::AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
{
   printf("EventDumpModule::AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
}

void EventDumpModuleFactory::Init(const std::vector<std::string> &args)
{
   if (gTrace)
      printf("EventDumpModuleFactory::Init!\n");
}

void EventDumpModuleFactory::Finish()
{
   if (gTrace)
      printf("EventDumpModuleFactory::Finish!\n");
}

TARunObject* EventDumpModuleFactory::NewRunObject(TARunInfo* runinfo)
{
   if (gTrace)
      printf("EventDumpModuleFactory::NewRunObject, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   return new EventDumpModule(runinfo);
}

// ==================== XButton Methods ==================== //

XButton::XButton(TGWindow*p, const char* text, XCtrl* ctrl, int value): TGTextButton(p, text)
{
   fCtrl = ctrl;
   fValue = value;
}

void XButton::Clicked()
{
   //printf("Clicked button %s, value %d!\n", GetString().Data(), fValue);
   if (fCtrl)
      fCtrl->fValue = fValue;
   //gSystem->ExitLoop();
}


// ==================== MainWindow Methods ==================== //

MainWindow::MainWindow(const TGWindow*w, int s1, int s2, XCtrl* ctrl): TGMainFrame(w, s1, s2)
{
   if (gTrace)
      printf("MainWindow::ctor!\n");

   fCtrl = ctrl;
   //SetCleanup(kDeepCleanup);

   SetWindowName("ROOT Analyzer Control");

   // layout the gui
   fMenu = new TGPopupMenu(gClient->GetRoot());
   fMenu->AddEntry("New TBrowser", CTRL_TBROWSER);
   fMenu->AddEntry("-", 0);
   fMenu->AddEntry("Next",     CTRL_NEXT);
   fMenu->AddEntry("Continue", CTRL_CONTINUE);
   fMenu->AddEntry("Pause",    CTRL_PAUSE);
   fMenu->AddEntry("-", 0);
   fMenu->AddEntry("Quit",     CTRL_QUIT);

   menuBarItemLayout = new TGLayoutHints(kLHintsTop|kLHintsLeft, 0, 4, 0, 0);

   fMenu->Associate(this);

   fMenuBar = new TGMenuBar(this, 1, 1, kRaisedFrame);
   fMenuBar->AddPopup("&Rootana", fMenu, menuBarItemLayout);
   fMenuBar->Layout();

   menuBarLayout = new TGLayoutHints(kLHintsTop|kLHintsLeft|kLHintsExpandX);
   AddFrame(fMenuBar, menuBarLayout);

   // Create a container frames containing buttons

   // one button is resized up to the parent width.
   // Note! this width should be fixed!
   TGVerticalFrame *hframe1 = new TGVerticalFrame(this, 170, 50, kFixedWidth);

   // to take whole space we need to use kLHintsExpandX layout hints
   hframe1->AddFrame(new XButton(hframe1, "&Next ", ctrl, CTRL_NEXT),
                     new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 0, 2, 2));
   AddFrame(hframe1, new TGLayoutHints(kLHintsCenterX, 2, 2, 5, 1));

   // two buttons are resized up to the parent width.
   // Note! this width should be fixed!
   TGCompositeFrame *cframe1 = new TGCompositeFrame(this, 170, 20, kHorizontalFrame | kFixedWidth);

   // to share whole parent space we need to use kLHintsExpandX layout hints
   cframe1->AddFrame(new XButton(cframe1, "&Continue", ctrl, CTRL_CONTINUE),
                     new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 2, 2, 2));

   cframe1->AddFrame(new XButton(cframe1, "&Pause", ctrl, CTRL_PAUSE),
                     new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 2, 2, 2));

   AddFrame(cframe1, new TGLayoutHints(kLHintsCenterX, 2, 2, 5, 1));

   // three buttons are resized up to the parent width.
   // Note! this width should be fixed!
   TGCompositeFrame *cframe2 = new TGCompositeFrame(this, 170, 20, kHorizontalFrame | kFixedWidth);

#if 0
   TGButton* ok = new XButton(cframe2, "OK", ctrl, 0);
   // to share whole parent space we need to use kLHintsExpandX layout hints
   cframe2->AddFrame(ok, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 3, 2, 2, 2));

   TGButton* cancel = new XButton(cframe2, "Cancel ", ctrl, 0);
   cframe2->AddFrame(cancel, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 3, 2, 2, 2));
#endif

   cframe2->AddFrame(new XButton(cframe2, "&Quit ", ctrl, CTRL_QUIT),
                     new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 0, 2, 2));

   AddFrame(cframe2, new TGLayoutHints(kLHintsCenterX, 2, 2, 5, 1));

   MapSubwindows();
   Layout();
   MapWindow();
}

MainWindow::~MainWindow() // dtor // Closing the control window closes the whole program
{
   if (gTrace)
      printf("MainWindow::dtor!\n");

   delete fMenu;
   delete fMenuBar;
   delete menuBarLayout;
   delete menuBarItemLayout;
}

void MainWindow::CloseWindow()
{
   if (gTrace)
      printf("MainWindow::CloseWindow()\n");

   if (fCtrl)
      fCtrl->fValue = CTRL_QUIT;
   //gSystem->ExitLoop();
}

Bool_t MainWindow::ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2)
{
   //printf("GUI Message %d %d %d\n",(int)msg,(int)parm1,(int)parm2);
   switch (GET_MSG(msg))
      {
      default:
         break;
      case kC_COMMAND:
         switch (GET_SUBMSG(msg))
            {
            default:
               break;
            case kCM_MENU:
               //printf("parm1 %d\n", (int)parm1);
               switch (parm1)
                  {
                  case CTRL_TBROWSER:
                     new TBrowser();
                     break;
                  default:
                     //printf("Control %d!\n", (int)parm1);
                     if (fCtrl)
                        fCtrl->fValue = parm1;
                     //gSystem->ExitLoop();
                     break;
                  }
               break;
            }
         break;
      }

   return kTRUE;
}

// ==================== InteractiveModule Methods ==================== //

InteractiveModule::InteractiveModule(TARunInfo* runinfo)
   : TARunObject(runinfo)
{
   if (gTrace)
      printf("InteractiveModule::ctor, run %d\n", runinfo->fRunNo);
   fContinue = false;
   fSkip = 0;
#ifdef HAVE_ROOT
   if (!fgCtrl)
      fgCtrl = new XCtrl;
   if (!fgCtrlWindow && runinfo->fRoot->fgApp) {
      fgCtrlWindow = new MainWindow(gClient->GetRoot(), 200, 300, fgCtrl);
   }
#endif
}

InteractiveModule::~InteractiveModule()
{
   if (gTrace)
      printf("InteractiveModule::dtor!\n");
}

void InteractiveModule::EndRun(TARunInfo* runinfo)
{
   printf("InteractiveModule::EndRun, run %d\n", runinfo->fRunNo);

#ifdef HAVE_ROOT
   if (fgCtrlWindow && runinfo->fRoot->fgApp) {
      while (1) {
#ifdef HAVE_THTTP_SERVER
         if (TARootHelper::fgHttpServer) {
            TARootHelper::fgHttpServer->ProcessRequests();
         }
#endif
#ifdef HAVE_ROOT
         if (TARootHelper::fgApp) {
            gSystem->DispatchOneEvent(kTRUE);
         }
#endif
#ifdef HAVE_MIDAS
         if (!TMidasOnline::instance()->sleep(10)) {
            // FIXME: indicate that we should exit the analyzer
            return;
         }
#else
         gSystem->Sleep(10);
#endif

         int ctrl = fgCtrl->fValue;
         fgCtrl->fValue = 0;

         switch (ctrl) {
         case CTRL_QUIT:
            return;
         case CTRL_NEXT:
            return;
         case CTRL_CONTINUE:
            return;
         }
      }
   }
#endif
}

TAFlowEvent* InteractiveModule::Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
{
   printf("InteractiveModule::Analyze, run %d, %s\n", runinfo->fRunNo, event->HeaderToString().c_str());

#ifdef HAVE_ROOT
   if (fgCtrl->fValue == CTRL_QUIT) {
      *flags |= TAFlag_QUIT;
      return flow;
   } else if (fgCtrl->fValue == CTRL_PAUSE) {
      fContinue = false;
   }
#endif

   if (fContinue && !(*flags & TAFlag_DISPLAY)) {
      return flow;
   } else {
      fContinue = false;
   }

   if (fSkip > 0) {
      fSkip--;
      return flow;
   }

#ifdef HAVE_ROOT
   if (fgCtrlWindow && runinfo->fRoot->fgApp) {
      while (1) {
#ifdef HAVE_THTTP_SERVER
         if (TARootHelper::fgHttpServer) {
            TARootHelper::fgHttpServer->ProcessRequests();
         }
#endif
#ifdef HAVE_ROOT
         if (TARootHelper::fgApp) {
            gSystem->DispatchOneEvent(kTRUE);
         }
#endif
#ifdef HAVE_MIDAS
         if (!TMidasOnline::instance()->sleep(10)) {
            *flags |= TAFlag_QUIT;
            return flow;
         }
#else
         gSystem->Sleep(10);
#endif

         int ctrl = fgCtrl->fValue;
         fgCtrl->fValue = 0;

         switch (ctrl) {
         case CTRL_QUIT:
            *flags |= TAFlag_QUIT;
            return flow;
         case CTRL_NEXT:
            return flow;
         case CTRL_CONTINUE:
            fContinue = true;
            return flow;
         }
      }
   }
#endif

   while (1) {
      char str[256];
      fprintf(stdout, "manalyzer> "); fflush(stdout);
      fgets(str, sizeof(str)-1, stdin);

      printf("command [%s]\n", str);

      if (str[0] == 'h') { // "help"
         printf("Interactive manalyzer commands:\n");
         printf(" q - quit\n");
         printf(" h - help\n");
         printf(" c - continue until next TAFlag_DISPLAY event\n");
         printf(" n - next event\n");
         printf(" aNNN - analyze N events, i.e. \"a10\"\n");
      } else if (str[0] == 'q') { // "quit"
         *flags |= TAFlag_QUIT;
         return flow;
      } else if (str[0] == 'n') { // "next"
         return flow;
      } else if (str[0] == 'c') { // "continue"
         fContinue = true;
         return flow;
      } else if (str[0] == 'a') { // "analyze" N events
         int num = atoi(str+1);
         printf("Analyzing %d events\n", num);
         if (num > 0) {
            fSkip = num-1;
         }
         return flow;
      }
   }

   return flow;
}

void InteractiveModule::AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
{
   if (gTrace)
      printf("InteractiveModule::AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
}


MainWindow* InteractiveModule::fgCtrlWindow = NULL;
XCtrl* InteractiveModule::fgCtrl = NULL;

// ==================== InteractiveModuleFactory Methods ==================== //

void InteractiveModuleFactory::Init(const std::vector<std::string> &args)
{
   if (gTrace)
      printf("InteractiveModuleFactory::Init!\n");
}

void InteractiveModuleFactory::Finish()
{
   if (gTrace)
      printf("InteractiveModuleFactory::Finish!\n");
}

TARunObject* InteractiveModuleFactory::NewRunObject(TARunInfo* runinfo)
{
   if (gTrace)
      printf("InteractiveModuleFactory::NewRunObject, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   return new InteractiveModule(runinfo);
}

static void help()
{
   printf("\nUsage:\n");
   printf("\n./analyzer.exe [-h] [-R8081] [-o Outputfile.root] [file1 file2 ...] [-- arguments passed to modules ...]\n");
   printf("\n");
   printf("   -h                  - print this help message\n");
   printf("   -H <hostname>       - connect to MIDAS experiment on given host\n");
   printf("   -E <exptname>       - connect to this MIDAS experiment\n");
   printf("   -o <Outputfile.root> - write to this file\n");
   printf("   -R <nnnn>           - Start the ROOT THttpServer HTTP server on specified tcp port,\n");
   printf("                         access by firefox http://localhost:8081\n");
   printf("   -X <nnnn>           - Start the Xml server on specified tcp port\n");
   printf("                         (for use with roody -Xlocalhost:9091)\n");
   printf("   -P <nnnn>           - Start the TNetDirectory server on specified tcp port\n");
   printf("                         (for use with roody -Plocalhost:9091)\n");
   printf("   -e <NNN>            - Number of events to analyze\n");
   printf("   -s <NNN>            - Number of events to skip before starting analysis\n");
   printf("   -t                  - Enable tracing of constructors, destructors and function calls\n");
   printf("   -m                  - Enable memory leak debugging\n");
   printf("   -g                  - Enable graphics display when processing data files\n");
   printf("   -i                  - Enable intractive mode\n");
   printf("   --dump              - activate the event dump module\n");
   printf("   --                  - All following arguments are passed to the analyzer modules Init() method\n");
   printf("\n");
   printf("   Example1            - analyze online data   :\t ./analyzer.exe -P9091\n");
   printf("   Example2            - analyze existing data :\t ./analyzer.exe /data/alpha/current/run00500.mid\n");
   exit(1);
}

// Main function call

int manalyzer_main(int argc, char *argv[])
{
   setbuf(stdout, NULL);
   setbuf(stderr, NULL);

   signal(SIGILL,  SIG_DFL);
   signal(SIGBUS,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
   signal(SIGPIPE, SIG_DFL);

   std::vector<std::string> args;
   for (int i=0; i<argc; i++) {
      if (strcmp(argv[i],"-h")==0)
         help(); // does not return
      args.push_back(argv[i]);
   }

   int  tcpPort = 0;
   int  xmlTcpPort = 0;
   int  httpPort = 0;
   const char* hostname = NULL;
   const char* exptname = NULL;

   int num_skip = 0;
   int num_analyze = 0;

   TMWriterInterface *writer = NULL;

   bool event_dump = false;
   bool root_graphics = false;
   bool interactive = false;

   std::vector<std::string> files;
   std::vector<std::string> modargs;

   for (unsigned int i=1; i<args.size(); i++) { // loop over the commandline options
      const char* arg = args[i].c_str();
      //printf("argv[%d] is %s\n",i,arg);

      if (args[i] == "--") {
         for (unsigned j=i+1; j<args.size(); j++)
            modargs.push_back(args[j]);
         break;
      } else if (args[i] == "--dump") {
         event_dump = true;
      } else if (args[i] == "-g") {
         root_graphics = true;
      } else if (args[i] == "-i") {
         interactive = true;
      } else if (args[i] == "-t") {
         gTrace = true;
         TMReaderInterface::fgTrace = true;
         TMWriterInterface::fgTrace = true;
      } else if (strncmp(arg,"-o",2)==0) {
         writer = TMNewWriter(arg+2);
      } else if (strncmp(arg,"-s",2)==0) {
         num_skip = atoi(arg+2);
      } else if (strncmp(arg,"-e",2)==0) {
         num_analyze = atoi(arg+2);
      } else if (strncmp(arg,"-m",2)==0) { // Enable memory debugging
         gEnableShowMem = true;
      } else if (strncmp(arg,"-P",2)==0) { // Set the histogram server port
         tcpPort = atoi(arg+2);
      } else if (strncmp(arg,"-X",2)==0) { // Set the histogram server port
         xmlTcpPort = atoi(arg+2);
      } else if (strncmp(arg,"-R",2)==0) { // Set the ROOT THttpServer HTTP port
         httpPort = atoi(arg+2);
      } else if (strncmp(arg,"-H",2)==0) {
         hostname = strdup(arg+2);
      } else if (strncmp(arg,"-E",2)==0) {
         exptname = strdup(arg+2);
      } else if (strcmp(arg,"-h")==0) {
         help(); // does not return
      } else if (arg[0] == '-') {
         help(); // does not return
      } else {
         files.push_back(args[i]);
      }
   }

   if (!gModules)
      gModules = new std::vector<TAFactory*>;

   if ((*gModules).size() == 0)
      event_dump = true;

   if (event_dump)
      (*gModules).push_back(new EventDumpModuleFactory);

   if (interactive)
      (*gModules).push_back(new InteractiveModuleFactory);

   printf("Registered modules: %d\n", (int)(*gModules).size());

#ifdef HAVE_ROOT
   if (root_graphics) {
      TARootHelper::fgApp = new TApplication("manalyzer", NULL, NULL, 0, 0);
   }

   TARootHelper::fgDir = new TDirectory("manalyzer", "location of histograms");
   TARootHelper::fgDir->cd();
#endif

#ifdef XHAVE_LIBNETDIRECTORY
   if (tcpPort) {
      VerboseNetDirectoryServer(true);
      StartNetDirectoryServer(tcpPort, TARootHelper::fgDir);
   }
#else
   if (tcpPort)
      fprintf(stderr,"ERROR: No support for the TNetDirectory server!\n");
#endif

#ifdef HAVE_XMLSERVER
   if (xmlTcpPort) {
      XmlServer* s = new XmlServer();
      s->SetVerbose(true);
      s->Start(xmlTcpPort);
      s->Export(gROOT, "ROOT");
      s->Export(TARootHelper::fgDir, "manalyzer");
      TARootHelper::fgXmlServer = s;
   }
#else
   if (xmlTcpPort)
      fprintf(stderr,"ERROR: No support for the XML Server!\n");
#endif

   if (httpPort) {
#ifdef HAVE_THTTP_SERVER
      char str[256];
      sprintf(str, "http:127.0.0.1:%d", httpPort);
      THttpServer *s = new THttpServer(str);
      //s->SetTimer(100, kFALSE);
      TARootHelper::fgHttpServer = s;
#else
      fprintf(stderr,"ERROR: No support for the THttpServer!\n");
#endif
   }

   for (unsigned i=0; i<files.size(); i++) {
      printf("file[%d]: %s\n", i, files[i].c_str());
   }

   if (files.size() > 0) {
      ProcessMidasFiles(files, modargs, num_skip, num_analyze, writer);
   } else {
#ifdef HAVE_MIDAS
      ProcessMidasOnline(modargs, hostname, exptname, num_analyze, writer);
#endif
   }

   if (writer) {
      writer->Close();
      delete writer;
      writer = NULL;
   }

   return 0;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
