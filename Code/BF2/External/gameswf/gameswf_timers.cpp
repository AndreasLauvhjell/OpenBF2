// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "base/smart_ptr.h"

#include "gameswf_timers.h"
#include "gameswf_xml.h"

using namespace std;

namespace gameswf 
{
  Timer::Timer() :
      _which(0),
      _interval(0.0),
      _start(0.0),
      _object(0),
      _env(0)
  {
  }
  
  Timer::Timer(as_value *obj, int ms)
  {
    setInterval(*obj, ms);
    start();
  }
  
  Timer::~Timer()
  {
    log_msg("%s: \n", __FUNCTION__);
  }
  
  int
  Timer::setInterval(as_value obj, int ms)
  {
    _function = obj;
    _interval = ms * 0.001;   // [DICE-MOD SwiffPlayer: milliseconds. The stock 0.01 made every interval ten times too long; the shipped timer keeps the raw ms.]
    // _interval = ms * 0.000001;
    start();

    return 0;
  }

  int
  Timer::setInterval(as_value obj, int ms, as_environment *en)
  {
    _function = obj;
    _interval = ms * 0.001;   // [DICE-MOD SwiffPlayer: milliseconds. The stock 0.01 made every interval ten times too long; the shipped timer keeps the raw ms.]
    _env = en;
    // _interval = ms * 0.000001;
    start();

    return 0;
  }
  int
  Timer::setInterval(as_value obj, int ms, array<struct variable *> *locals)
  {
    _function = obj;
    _interval = ms * 0.001;   // [DICE-MOD SwiffPlayer: milliseconds. The stock 0.01 made every interval ten times too long; the shipped timer keeps the raw ms.]
    _locals = locals;
    // _interval = ms * 0.000001;
    start();

    return 0;
  }

  int
  Timer::setInterval(as_value obj, int ms, as_object_interface *this_ptr, as_environment *en)
  {
    _function = obj;
    _interval = ms * 0.001;   // [DICE-MOD SwiffPlayer: milliseconds. The stock 0.01 made every interval ten times too long; the shipped timer keeps the raw ms.]
    _env = en;
    _object = this_ptr;
    // _interval = ms * 0.000001;
    start();

    return 0;
  }

  void
  Timer::clearInterval()
  {
    _interval = 0.0;
    _start = 0.0;
  }
  
  void
  Timer::start()
  {
    uint64 ticks = tu_timer::get_profile_ticks();
    _start = tu_timer::profile_ticks_to_seconds(ticks);
  }
  

  bool
  Timer::expired()
  {
    if (_start > 0.0) {
      uint64 ticks = tu_timer::get_profile_ticks();
      double now = tu_timer::profile_ticks_to_seconds(ticks);
      //printf("FIXME: %s: now is %f, start time is %f, interval is %f\n", __FUNCTION__, now, _start, _interval);
      if (now > _start + _interval) {
        _start = now;               // reset the timer
        //log_msg("Timer expired! \n");
        return true;
      }
      // FIXME: Sometimes, "now" and "_start" have bad values.
      // I don't know why, but this works around the problem..
      else if (now < _start) {
        log_msg( "Timer::expired - now (%lf) is before start (%lf)!\n"
                 "     Expiring right now.\n",
                 now, _start);
        _start = now;
        return true;
      }
    }
      
    // log_msg("Timer not enabled! \n");
    return false;
  }

  void
  timer_setinterval(const fn_call& fn)
  {
    int i;
    as_value	method;
    log_msg("%s: args=%d\n", __FUNCTION__, fn.nargs);
    
    timer_as_object *ptr = new timer_as_object;
    
    //  action_buffer	m_action_buffer;
    //timer_as_object*	ptr = (timer_as_object*) (as_object*) this_ptr;
    assert(ptr);
    
    movie*	mov = fn.env->get_target()->get_root_movie();
    // [DICE-MOD SwiffPlayer: both AS2 forms. The shipped gameswf_as_global_setinterval branches on
    // whether arg0 converts to an object: setInterval(object, "methodName", ms, args...) resolves
    // the method on that object and ticks with it as `this`; otherwise setInterval(function, ms,
    // args...). Stock 2006 knew only the function form and, handed an object, stored the OBJECT as
    // the "function" - every tick then logged "error in call_method(): method is not a function"
    // (retail 2026-09-02: once per frame, from the menu's update manager).]
    as_value val;
    as_object_interface* target = NULL;
    int ms = 0;
    {
      const as_value& first = fn.env->bottom(fn.first_arg_bottom_index);
      if (first.to_as_function() == NULL && first.to_object() != NULL && fn.nargs >= 3)
      {
        target = first.to_object();
        const tu_string method_name = fn.env->bottom(fn.first_arg_bottom_index - 1).to_tu_string();
        target->get_member(tu_stringi(method_name.c_str()), &val);
        ms = (int) fn.env->bottom(fn.first_arg_bottom_index - 2).to_number();
      }
      else
      {
        as_as_function *as_func = (as_as_function *)first.to_as_function();
        val = as_value(as_func);
        ms = (int) fn.env->bottom(fn.first_arg_bottom_index - 1).to_number();
      }
    }

    // [TEMP] RW-89 timer trace. Remove with the tracers.
    log_error("[setInterval] form=%s method_type=%d ms=%d target=%p\n",
              target ? "object" : "function", (int) val.get_type(), ms, target);

    tu_string local_name;
    as_value local_val;
    array<with_stack_entry>	dummy_with_stack;

    fn.env->add_frame_barrier();
    //method = env->get_variable("loopvar", dummy_with_stack);

#if 1
    // FIXME: This is pretty gross, but something is broke elsewhere and it doesn't
    // seem to effect anything else. When a function is called from a executing
    // function, like calling setInterval() from within the callback to
    // XMLSocket::onConnect(), the local variables of the parent function need to
    // be propogated to the local stack as regular variables (not locals) or
    // they can't be found in the scope of the executing chld function. There is
    // probably a better way to do this... but at least this works.
    for (i=0; i< fn.env->get_local_frame_top(); i++) {
      if (fn.env->m_local_frames[i].m_name.size()) {
        //method = env->get_variable(env->m_local_frames[i].m_name, dummy_with_stack);
        //if (method.get_type() != as_value::UNDEFINED)
        {
          local_name  = fn.env->m_local_frames[i].m_name;
          local_val = fn.env->m_local_frames[i].m_value;
          fn.env->set_variable(local_name, local_val, 0);
        }
      }
    }
#endif
    //    ptr->obj.setInterval(val, ms, (as_object *)ptr, env);
    
    //Ptr->obj.setInterval(val, ms);
    ptr->obj.setInterval(val, ms, target ? target : (as_object_interface *)(as_object *)ptr, fn.env);
    
    fn.result->set_int(mov->add_interval_timer(&ptr->obj));
  }
  
  void
  timer_expire(const fn_call& fn)
  {
    //log_msg("%s:\n", __FUNCTION__);

    timer_as_object*	ptr = (timer_as_object*) (as_object*) fn.this_ptr;
    assert(ptr);
    const as_value&	val = ptr->obj.getASFunction();
    
    if (as_as_function* as_func = val.to_as_function()) {
      // It's an ActionScript function.  Call it.
      log_msg("Calling ActionScript function for setInterval Timer\n");
      (*as_func)(fn_call(fn.result, fn.this_ptr, fn.env, 0, 0));
    } else {
      log_error("FIXME: Couldn't find setInterval Timer!\n");
    }
  }
  
  void
  timer_clearinterval(const fn_call& fn)
  {
    //log_msg("%s: nargs = %d\n", __FUNCTION__, nargs);

    double id = fn.env->bottom(fn.first_arg_bottom_index).to_number();

    movie*	mov = fn.env->get_target()->get_root_movie();
    // [TEMP] RW-89 timer trace. Remove with the tracers.
    log_error("[clearInterval] id=%g\n", id);
    mov->clear_interval_timer((int)id);
    fn.result->set_bool(true); 
  }
}
