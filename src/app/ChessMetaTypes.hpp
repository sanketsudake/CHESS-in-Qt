#pragma once

#include "chess/Move.hpp"
#include "chess/Rules.hpp"
#include "chess/Types.hpp"

#include <QMetaType>

// Introduces the rules library's types to Qt's meta-object system.
//
// The rules know nothing about Qt, by design, so the declarations cannot live
// beside the types themselves. Without them a queued connection cannot carry
// these types across a thread and QSignalSpy cannot record them.
//
// Every Qt class that names one of these types in a signal or slot must
// include this header. Qt compiles all of a target's moc output into one
// translation unit, so a class whose moc instantiates QMetaTypeId before this
// declaration is seen fails to compile with "explicit specialization after
// instantiation" -- and which class that is depends on alphabetical order,
// which makes it a confusing failure to meet by surprise.
Q_DECLARE_METATYPE(chess::Square)
Q_DECLARE_METATYPE(chess::Color)
Q_DECLARE_METATYPE(chess::PieceType)
Q_DECLARE_METATYPE(chess::Move)
Q_DECLARE_METATYPE(chess::Outcome)
Q_DECLARE_METATYPE(chess::TerminalReason)
