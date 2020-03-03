from copy import deepcopy
import wrapt

### Global list to keep track of shared objects.
# Don't touch if you're not inside this module.
porpals_list = []

### Dummy class used to verify if something has already been shared
class PorpalSigniture(wrapt.ObjectProxy):
    pass

### TODO: This needs to get fixed
def copy():
    # Pause recording, copy should not make logs
    lst = []
    for x in porpals_list:
        lst.append(deepcopy(strip(x)))

    return lst

def strip(instance):
    return instance.__wrapped__

### Merges porpals
def merge(old, joined):
    for i in range(len(porpals_list)):
        porpals_list[i].porpal_merge(old[i], joined[i])

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
