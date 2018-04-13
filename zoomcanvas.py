from fltk import *

class ZoomCanvas(Ca_Canvas):
        def __init__(self, x,y,w,h,s=''):
                # zoomstack elements will be [x1,y1,x2,y2], [etc]
                # Just in case it is not initialised properly...
                self.zoomstack = [[0,0,1,1]]
                Ca_Canvas.__init__(self, x,y,w,h,s)
        
        def initzoom(self, xmin, ymin, xmax, ymax):
                '''Set the initial limits and put these on the stack'''
                if xmin == xmax or ymin == ymax: return
                self.zoomstack = [[xmin, ymin, xmax, ymax]]
                self.current_x().minimum(xmin)
                self.current_y().minimum(ymin)
                self.current_x().maximum(xmax)
                self.current_y().maximum(ymax)
        
        def pushzoom(self, xmin, ymin, xmax, ymax):
                # push the existing limits, then set new limits
                if xmin == xmax or ymin == ymax: return
                self.zoomstack.append([self.current_x().minimum(), self.current_y().minimum(),
                                       self.current_x().maximum(), self.current_y().maximum()])
                self.current_x().minimum(xmin)
                self.current_y().minimum(ymin)
                self.current_x().maximum(xmax)
                self.current_y().maximum(ymax)
        
        def popzoom(self):
                r = self.zoomstack[-1]   # always want the last on stack
                if len(self.zoomstack) > 1:  # pop unless it is the init value
                        self.zoomstack = self.zoomstack[:-1]
                return r
        
        def firstzoom(self):
                r = self.zoomstack[0]
                self.zoomstack = [r]
                return r

        def handle(self, event):
                if event == FL_PUSH:
                        if Fl.event_button() == 1:   # L button: start a zoom drag
                                self.dragstartx = Fl.event_x()
                                self.dragstarty = Fl.event_y()
                        elif Fl.event_button() == 3:  # R button: revert to previous zoom
                                res = self.popzoom()
                                self.current_x().minimum(res[0])
                                self.current_y().minimum(res[1])
                                self.current_x().maximum(res[2])
                                self.current_y().maximum(res[3])
                        elif Fl.event_button() == 2:  # M button: erase zoom history, go to first
                                res = self.firstzoom()
                                self.current_x().minimum(res[0])
                                self.current_y().minimum(res[1])
                                self.current_x().maximum(res[2])
                                self.current_y().maximum(res[3])
                        return 1
                elif event == FL_RELEASE and Fl.event_button() == 1: # end of zoom drag
                        fl_overlay_clear()
                        
                        x1 = self.current_x().value(self.dragstartx)
                        x2 = self.current_x().value(Fl.event_x())
                        y1 =self.current_y().value(self.dragstarty)
                        y2 = self.current_y().value(Fl.event_y())
                        
                        if x1 == x2 or y1 == y2: return 1
                        
                        self.pushzoom(min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2))
                        return 1
                        
                elif event == FL_DRAG and Fl.event_button() == 1:
                        self.window().make_current()
                        fl_overlay_rect(self.dragstartx,self.dragstarty, 
                           Fl.event_x()-self.dragstartx, Fl.event_y()-self.dragstarty )
                        return 1
                else:
                        return 0
        def draw(self):
                fl_overlay_clear()
                Ca_Canvas.draw(self)
