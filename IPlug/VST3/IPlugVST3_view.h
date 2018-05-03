#pragma once
using namespace Steinberg;
using namespace Vst;

/** IPlug VST3 View  */
class IPlugVST3View : public CPluginView
{
public:
  IPlugVST3View(IPlugVST3Controller* pController)
  : mController(pController)
  , mExpectingNewSize(false)
  {
    if (mController)
      mController->addRef();
  }
  
  ~IPlugVST3View()
  {
    if (mController)
    {
      mController->release();
    }
  }
  
  tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override
  {
    if(mController->HasUI()) // for no editor plugins
    {
#ifdef OS_WIN
      if (strcmp(type, kPlatformTypeHWND) == 0)
        return kResultTrue;
      
#elif defined OS_MAC
      if (strcmp (type, kPlatformTypeNSView) == 0)
        return kResultTrue;
      else if (strcmp(type, kPlatformTypeHIView) == 0)
        return kResultTrue;
#endif
    }
    
    return kResultFalse;
  }
  
  tresult PLUGIN_API onSize(ViewRect* pSize) override
  {
    TRACE;
    
    if (pSize)
    {
      rect = *pSize;
      
      if (mExpectingNewSize)
      {
        mController->OnWindowResize();
        mExpectingNewSize = false;
      }
    }
    
    return kResultTrue;
  }
  
  tresult PLUGIN_API getSize(ViewRect* pSize) override
  {
    TRACE;
    
    if (mController->HasUI())
    {
      *pSize = ViewRect(0, 0, mController->Width(), mController->Height());
      
      return kResultTrue;
    }
    else
    {
      return kResultFalse;
    }
  }
  
  tresult PLUGIN_API attached(void* pParent, FIDString type) override
  {
    if (mController->HasUI())
    {
#ifdef OS_WIN
      if (strcmp(type, kPlatformTypeHWND) == 0)
        mController->OpenWindow(pParent);
#elif defined OS_MAC
      if (strcmp (type, kPlatformTypeNSView) == 0)
        mController->OpenWindow(pParent);
      else // Carbon
        return kResultFalse;
#endif
      mController->OnUIOpen();
      
      return kResultTrue;
    }
    
    return kResultFalse;
  }
  
  tresult PLUGIN_API removed() override
  {
    if (mController->HasUI())
    {
      mController->OnUIClose();
      mController->CloseWindow();
    }
    
    return CPluginView::removed();
  }
  
  void resize(int w, int h)
  {
    TRACE;
    
    ViewRect newSize = ViewRect(0, 0, w, h);
    mExpectingNewSize = true;
    plugFrame->resizeView(this, &newSize);
  }
  
  IPlugVST3Controller* mController;
  bool mExpectingNewSize = false;
};