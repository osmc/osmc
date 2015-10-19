/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "AddonClass.h"
#include "CallbackHandler.h"

#include "threads/Event.h"

/**
 * This class is an interface that can be used to define programming language
 * specific hooks.
 */

class IPlayerCallback;

namespace XBMCAddon
{
  namespace xbmc
  {
    class Monitor;
  }

  class LanguageHook : public AddonClass
  {
  protected:
    inline LanguageHook() {}

  public:
    virtual ~LanguageHook();

    /**
     * If the scripting language needs special handling for calls 
     *  that block or are delayed in any other means, this should
     *  be overloaded.
     *
     * In Python when control is passed to a native
     *  method that blocks, you need to allow other threads in 
     *  Python to run by using Py_BEGIN_ALLOW_THREADS. This is
     *  the place to put that functionality
     */
    virtual void DelayedCallOpen() { }

    /**
     * If the scripting language needs special handling for calls 
     *  that block or are delayed in any other means, this should
     *  be overloaded.
     *
     * In Python when control is passed to a native
     *  method that blocks, you need to allow other threads in 
     *  Python to run by using Py_BEGIN_ALLOW_THREADS. When that
     *  delayed method ends you need to restore the Python thread
     *  state using Py_END_ALLOW_THREADS. This is the place to put
     *  that functionality
     */
    virtual void DelayedCallClose() { }

    virtual void MakePendingCalls() {}

    /**
     * For scripting languages that need a global callback handler, this
     *  method should be overloaded to supply one.
     */
    virtual CallbackHandler* GetCallbackHandler() { return NULL; }

    /**
     * This is a callback method that can be overriden to receive a callback
     *  when an AddonClass that has this language hook is on is being constructed.
     *  It is called from the constructor of AddonClass so the implementor
     *  cannot assume the subclasses have been built or that calling a
     *  virtual function on the AddonClass will work as expected.
     */
    virtual void Constructing(AddonClass* beingConstructed) { }

    /**
     * This is a callback method that can be overriden to receive a callback
     *  when an AddonClass that has this language hook is on is being destructed.
     *  It is called from the destructor of AddonClass so the implementor
     *  should assume the subclasses have been torn down and that calling a
     *  virtual function on the AddonClass will not work as expected.
     */
    virtual void Destructing(AddonClass* beingDestructed) { }

    /**
     * This method should be done a different way but since the only other way
     *  I can think to do it requires an InheritableThreadLocal C++ equivalent,
     *  I'm going to defer to this technique for now.
     *
     * Currently (for python) the scripting languge has the Addon id injected
     *  into it as a global variable. Therefore the only way to retrieve it is
     *  to use scripting language specific calls. So until I figure out a 
     *  better way to do this, this is how I need to retrieve it.
     */
    virtual String GetAddonId() { return emptyString; }
    virtual String GetAddonVersion() { return emptyString; }

    virtual void RegisterPlayerCallback(IPlayerCallback* player) = 0;
    virtual void UnregisterPlayerCallback(IPlayerCallback* player) = 0;
    virtual void RegisterMonitorCallback(XBMCAddon::xbmc::Monitor* player) = 0;
    virtual void UnregisterMonitorCallback(XBMCAddon::xbmc::Monitor* player) = 0;
    virtual bool WaitForEvent(CEvent& hEvent, unsigned int milliseconds) = 0;

    static void SetLanguageHook(LanguageHook* languageHook);
    static LanguageHook* GetLanguageHook();
    static void ClearLanguageHook();
  };

  /**
   * This class can be used to access the language hook's DelayedCallOpen
   *  and DelayedCallClose. It should be used whenever an API method 
   *  is written such that it can block for an indefinite amount of time
   *  since certain scripting languages (like Python) need to do extra 
   *  work for delayed calls (like free the python locks and handle 
   *  callbacks).
   */
  class DelayedCallGuard
  {
    LanguageHook* languageHook;
    bool clearOnExit;

  public:
    inline DelayedCallGuard(LanguageHook* languageHook_) : languageHook(languageHook_), clearOnExit(false)
    { if (languageHook) languageHook->DelayedCallOpen(); }

    inline DelayedCallGuard() : languageHook(LanguageHook::GetLanguageHook()), clearOnExit(false) 
    { if (languageHook) languageHook->DelayedCallOpen(); }

    inline ~DelayedCallGuard()
    {
      if (clearOnExit) LanguageHook::ClearLanguageHook();
      if (languageHook) languageHook->DelayedCallClose();
    }

    inline LanguageHook* getLanguageHook() { return languageHook; }
  };

  class SetLanguageHookGuard
  {
  public:
    inline SetLanguageHookGuard(LanguageHook* languageHook) { LanguageHook::SetLanguageHook(languageHook); }
    inline ~SetLanguageHookGuard() { LanguageHook::ClearLanguageHook(); }
  };
  
}

