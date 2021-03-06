// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------
// Includes
//------------------------------------------
#include "customevents.h"
#include <QEvent>

//-------------------------------------------
// Forward declarations
//------------------------------------------
class ScriptingEnv;

/**
 * A custom event to notify an object that it should update its scripting
 * environment
 */
class ScriptingChangeEvent : public QEvent {
public:
  explicit ScriptingChangeEvent(ScriptingEnv *e)
      : QEvent(SCRIPTING_CHANGE_EVENT), env(e) {}
  ScriptingEnv *scriptingEnv() const { return env; }
  Type type() const { return SCRIPTING_CHANGE_EVENT; }

private:
  ScriptingEnv *env;
};

/**
 * An interface to the current scripting environment.
 *
 * Every class that wants to use a ScriptingEnv should subclass this one and
 * implement slot customEvent(QEvent*) such that it forwards any
 * ScriptingChangeEvents to Scripted::scriptingChangeEvent.
 */
class Scripted {
public:
  /// Constructor
  explicit Scripted(ScriptingEnv *env);
  /// Destructor
  ~Scripted();
  /// Called when the scripting environment changes
  void scriptingChangeEvent(ScriptingChangeEvent *);
  /// Access the current environment
  ScriptingEnv *scriptingEnv() { return m_scriptEnv; }

private:
  /// A pointer to the current environment
  ScriptingEnv *m_scriptEnv;
};
