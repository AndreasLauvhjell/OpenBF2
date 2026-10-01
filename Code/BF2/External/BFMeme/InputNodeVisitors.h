// Names/path [INFERRED]. Six complete three-slot visitors, client94F04C..94F090.
// These do not publish the incomplete Node family.
#ifndef BFMEME_INPUTNODEVISITORS_H
#define BFMEME_INPUTNODEVISITORS_H
#include "Node.h"
#if !DEDICATED
namespace dice { namespace meme {
// No receiver/null guard; source helper name is [INFERRED].
bool dispatchNodeEvent(System*, Node*, const Rect*, const NodeState*, const NodeInputEvent*);

class Event1NodeVisitor : public NodeVisitor
{
public:
    Event1NodeVisitor(int first, int index, int second);
    void visit(System*, Node*&, const Rect*, NodeState*) override;
private:
    int m_first, m_index, m_second;
    bool m_handled;
};
class Event2NodeVisitor : public NodeVisitor
{
public:
    Event2NodeVisitor(int first, int index);
    void visit(System*, Node*&, const Rect*, NodeState*) override;
private:
    int m_first, m_index;
};
class Event4NodeVisitor : public NodeVisitor
{
public:
    Event4NodeVisitor(int first, int index, int second);
    void visit(System*, Node*&, const Rect*, NodeState*) override;
private:
    int m_first, m_index, m_second;
};
class Event5NodeVisitor : public NodeVisitor
{
public:
    explicit Event5NodeVisitor(int first);
    void visit(System*, Node*&, const Rect*, NodeState*) override;
private:
    int m_first;
    // Shipped defect: inline constructor83BBBA..83BBE9 never writes this word;
    // visit reads it as the event index. Do not invent a default value.
    int m_index;
};
class PressStateNodeVisitor : public NodeVisitor
{
public:
    PressStateNodeVisitor();
    void visit(System*, Node*&, const Rect*, NodeState*) override;
};
class ReleaseStateNodeVisitor : public NodeVisitor
{
public:
    ReleaseStateNodeVisitor();
    void visit(System*, Node*&, const Rect*, NodeState*) override;
};
} }
#endif
#endif
