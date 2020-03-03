from copy import deepcopy
import wrapt
import inspect

class PorpalError(Exception):
    """Base Class for exceptions in this module"""
    pass


### Global list to keep track of shared objects.
# Don't touch if you're not inside this module.
porpals_list = []

porpals_record = False

def record():
    global porpals_record
    porpals_record = True

def pause():
    global porpals_record
    porpals_record = False

### TODO: This needs to get fixed
def copy():
    # Pause recording, copy should not make logs
    state = False
    if porpals_record:
        pause()
        state = True
    lst = []
    for x in porpals_list:
        lst.append(strip(x))

    # Continue recording if we were
    if state:
        record()
    return lst

### Merges porpals
# Cleans out the logs
def merge(old, joined):
    for i in range(len(porpals_list)):
        porpals_list[i].porpal_merge(old[i], joined[i])

### Clear the logs in a porpal
def clear_logs(instance):
    # Take especial care of built-ins
    if isinstance(instance, tuple, frozenset, list, set):
        for i in obj:
            clear_logs(i)
    elif isinstance(instance, dict):
        for i in obj.values():
            clear_logs(i)

    if hasattr(obj, '__dict__'):
        for i in obj.__dict__.values():
            clear_logs(i)

    instance._self_log.clear()

### Strip a class of its underlying wrappers
def strip(instance):
    obj = instance.__class__(instance)
    if isinstance(obj, tuple):
        lst = []
        for i in obj:
            lst.append(strip(i))
        obj = tuple(lst)
    elif isinstance(obj, frozenset):
        lst = []
        for i in obj:
            lst.append(strip(i))
        obj = frozenset(lst)
    # Take care of other sequences now
    elif isinstance(obj, list):
        for i in range(len(obj)):
            obj[i] = strip(obj[i])
    elif isinstance(obj, set):
        lst = list(obj)
        obj.clear()
        for i in lst:
            obj.add(strip(i))
    elif isinstance(obj, dict):
        for i in obj.keys():
            obj[i] = strip(obj[i])

    if hasattr(obj, '__dict__'):
        for i in obj.__dict__:
            obj.__dict__[i] = strip(obj.__dict__[i])

    return obj

### This function get an instance and returns a wrapped one.
# Objects produced here will not be logged. They will be simply returned
# And merged.
def porpal(instance):
	# Don't double wrap
	if not isinstance(instance, PorpalSigniture):
		# Make a simple signiture so it fails on future wrap attempts
		ret = PorpalSigniture(instance)
		# Only add it to the list if it's not already there
		porpals_list.append(ret)
	else:
		# If it is already a porpal, do nothing
		ret = instance
	return ret

### This function get an instance, a pre method call function,
# a post method call function, and an attribute set function.
# Objects produced here will have their own log at _self_log.
# All objects in its __dict__ will also be transformed into porpals.
# Note that in order for the loggin to be successful, objects need to be
# accessed through the returned wrapper, otherwise method calls and attribute
# changes will not be logged.
def porpal_log(instance, fpre, fpost, aset):
	# Don't double wrap
	if not isinstance(instance, PorpalSigniture):
		# Make a simple signiture so it fails on future wrap attempts
		ret = porpal_kernel(instance, fpre, fpost, aset)
		# Only add it to the list if it's not already there
		porpals_list.append(ret)
	else:
		# If it is already a porpal, do nothing
		ret = instance

	return ret

### A kernel to the porpal_logger. Almost similar build, but produces an
# extra list for the log and does not add to the global list
def porpal_kernel(instance, fpre, fpost, aset):
	# Don't double wrap
	if not isinstance(instance, PorpalSigniture):
		# Make a simple signiture so it fails on future wrap attempts
		ret = PorpalLogger(instance, fpre, fpost, aset, [])
	else:
		# If it is already a porpal, do nothing
		ret = instance

	return ret

### Produces a wrapper function to work with FunctionWrapper.
# Only does does a wrape on a method, anything else will happen without interception
def wrap_methods(*, fpre, fpost, aset, log):
    def wrapper(wrapped, instance, args, kwargs):
        if instance is None:
            # A class or function or static method
            return wrapped(*args, **kwargs)
        else:
            if inspect.isclass(instance):
                # A class method
                return wrapped(*args, **kwargs)
            else:
                # An instance method
                if porpals_record:
                    fret = fpre(wrapped, instance, args, kwargs)
                    if fret is not None:
                        log.append(fret)

                ret = wrapped(*args, **kwargs)
                if porpals_record:
                    fret = fpost(ret)
                    if fret is not None:
                        log.append(fret)

                porpal_kernel(instance, fpre, fpost, aset)
                return ret

    return wrapper

### Dummy class used to verify if something has already been shared
class PorpalSigniture(wrapt.ObjectProxy):
    pass

### The actual class for logged globals
class PorpalLogger(PorpalSigniture):
	def __init__(self, obj, fpre, fpost, aset, olog):
		# Take especial care of built-ins
		# First do the immutables sequences
		if isinstance(obj, tuple):
			lst = []
			for i in obj:
				lst.append(porpal_kernel(i, fpre, fpost, aset))
			obj = tuple(lst)
		elif isinstance(obj, frozenset):
			lst = []
			for i in obj:
				lst.append(porpal_kernel(i, fpre, fpost, aset))
			obj = frozenset(lst)
		# Take care of other sequences now
		elif isinstance(obj, list):
			for i in range(len(obj)):
				obj[i] = porpal_kernel(obj[i], fpre, fpost, aset)
		elif isinstance(obj, set):
			lst = list(obj)
			obj.clear()
			for i in lst:
				obj.add(porpal_kernel(i, fpre, fpost, aset))
		elif isinstance(obj, dict):
			for i in obj.keys():
				obj[i] = porpal_kernel(obj[i], fpre, fpost, aset)

		# Construct object now
		super().__init__(obj)
		super().__setattr__('_self_log', olog)
		super().__setattr__('_self_wrapper', wrap_methods(fpre=fpre, fpost=fpost, aset=aset, log=olog))
		super().__setattr__('_self_aset', aset)
		super().__setattr__('_self_fpre', fpre)
		super().__setattr__('_self_fpost', fpost)
		# If there is a dict, turn all members into PorpalWrappers
		if hasattr(obj, '__dict__'):
			for i in obj.__dict__:
				obj.__dict__[i] = porpal_kernel(obj.__dict__[i], fpre, fpost, aset)

		# TODO: Get all other stuff, turn them into PorpalWrappers

	# Intercept at method calls and wrap them.
	def __getattribute__(self, item):
		i = super().__getattribute__(item)
		if callable(i):
			## TODO: add lazy version of this
			return wrapt.FunctionWrapper(wrapped=i, wrapper=super().__getattribute__('_self_wrapper'))
		else:
			return i

	# Intercept at attribute sets and record
	def __setattr__(self, name, value):
		if porpals_record:
			ret = super().__getattribute__('_self_aset')(self, 'attr', name, value)
			if ret is not None:
				super().__getattribute__('_self_log').append(ret)
		super().__setattr__(name, porpal_kernel(value, super().__getattribute__('_self_fpre'),
													   super().__getattribute__('_self_fpost'),
													   super().__getattribute__('_self_aset')))

	# Intercept at attribute dels and record
	def __delattr__(self, name):
		if porpals_record:
			ret = super().__getattribute__('_self_aset')(self, 'del', name, None)
			if ret is not None:
				super().__getattribute__('_self_log').append(ret)
		super().__delattr__(name)

	# Intercept at item sets and record
	def __setitem__(self, key, value):
		if porpals_record:
			ret = super().__getattribute__('_self_aset')(self, 'item', key, value)
			if ret is not None:
				super().__getattribute__('_self_log').append(ret)
		super().__setitem__(key, porpal_kernel(value, super().__getattribute__('_self_fpre'),
													  super().__getattribute__('_self_fpost'),
													  super().__getattribute__('_self_aset')))
