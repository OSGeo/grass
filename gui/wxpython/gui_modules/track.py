"""
Variables set at initialization
"""
global curr_disp
curr_disp=""
global ctrl_dict
ctrl_dict={}

class Track:
	"""
	This class has functions and variables for tracking map display,
	associated notebook pages, and other index values.
	"""

	def SetCtrlDict(self, idx, disp, page, tree):
		"""
		This method stores the associated display index, display ID,
		controls book page ID, and map layer tree ID in a dictionary
		"""
		global ctrl_dict
		ctrl_dict[idx]=[disp, page, tree]
		return ctrl_dict

	def GetCtrls(self, idx, num):
		"""
		Returns widget ID for map display (num=0), controls book page
		(num=1), or map layer tree (num=2) for a given map display
		index (idx)
		"""
		global ctrl_dict
		ctrls = ctrl_dict[idx][num]
		return ctrls

	def popCtrl(self, idx):
		"""
		Removes entry from display and control dictionary.
		Used when display is closed
		"""
		global ctrl_dict
		if ctrl_dict != "":
			ctrl_dict.pop(idx)

	def GetDisp_idx(self, ctrl_id):
		"""
		Returns the display index for the display/controls dictionary entry
		given the widget ID of the control (ctrl_id)
		"""
		global ctrl_dict
		for idx,ctrl in ctrl_dict.items():
			if ctrl_id in ctrl:
				return idx

	def SetDisp(self, disp_idx, disp_id):
		"""
		Creates a tuple of the currently active display index and its corresponding
		map display frame ID.
		"""
		global curr_disp
		curr_disp = (disp_idx, disp_id)
		return curr_disp

	def GetDisp(self):
		"""
		Returns the (display index, display ID) tuple of the currently
		active display
		"""
		global curr_disp
		return curr_disp

