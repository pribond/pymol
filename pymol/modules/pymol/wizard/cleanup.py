import sys
import time
import os
from pymol.wizard import Wizard
from pymol import cmd
import pymol

SZYBKI_EXE = ""

undo_object = "_w_cleanup_undo"
redo_object = "_w_cleanup_redo"

def auto_configure():
    result = -1
    
    global SZYBKI_EXE
    OE_DIR = os.environ.get("OE_DIR",None)
    if OE_DIR == None:
        OE_DIR = os.environ.get("OEDIR",None)
    if OE_DIR != None:
        SZYBKI_EXE = os.path.join(OE_DIR,"bin/szybki")
        if os.path.exists(SZYBKI_EXE):
            result = 1
        else:
            SZYBKI_EXE = SZYBKI_EXE + ".exe"
#         print SZYBKI_EXE
            if os.path.exists(SZYBKI_EXE):
                result=1
        if result<0:
            SZYBKI_EXE = os.path.join(OE_DIR,"arch/microsoft-win32-i586/bin/szybki-1.0b7.exe")
            if os.path.exists(SZYBKI_EXE):
                 result = 1
	
    return result

class Cleanup(Wizard):

    def update_menus(self):
        ligand = [ [2, 'Ligand', '']]
        target = [ [2, 'Target', ''], [1, '(none)',' cmd.get_wizard().set_target("(none)")' ]]
        
        for a in cmd.get_names("public_objects"):
            ligand.append([1, a, 'cmd.get_wizard().set_ligand("'+a+'")'])
            if a!=self.ligand:
                target.append([1, a, 'cmd.get_wizard().set_target("'+a+'")'])
        self.menu['ligand'] = ligand
        self.menu['target'] = target

    def __init__(self,_self=cmd):
        Wizard.__init__(self,_self)
        
        self.ligand = ""
        for a in cmd.get_names("public_objects",1):
            if cmd.count_atoms(a) < 1000:
                self.ligand = a
                break
        self.target = "(none)"
        self.message = []
        self.menu = {}
        self.update_menus()
        cmd.edit_mode()

    def save_undo(self):
        cmd.delete(undo_object)
        cmd.create(undo_object,self.ligand,zoom=0)
        cmd.disable(undo_object)

    def undo(self):
        if undo_object in cmd.get_names("objects"):
            if self.ligand in cmd.get_names("objects"):
                cmd.delete(redo_object)
                cmd.set_name(self.ligand,redo_object)
                cmd.disable(redo_object)            
            cmd.create(self.ligand,undo_object,zoom=0)

    def redo(self):
        if redo_object in cmd.get_names("objects"):
            if self.ligand in cmd.get_names("objects"):
                cmd.delete(undo_object)
                cmd.set_name(self.ligand,undo_object)
                cmd.disable(undo_object)
            cmd.create(self.ligand,redo_object,zoom=0)
        
    def run(self):
        exe = SZYBKI_EXE
        inp = "ligand_inp.mol"
        out = "ligand_out.mol"
        tmp_obj = "_cleanup_tmp_obj"
        
        if self.ligand in cmd.get_names("objects"):
            self.save_undo()
            if os.path.exists(inp): os.unlink(inp)
            if os.path.exists(out): os.unlink(out)
            cmd.save(inp,self.ligand) # MOL file will be written using internal order
            while 1:
                if not os.path.exists(inp):
                    time.sleep(0.05)
                else:
                    break;
            cmd.system(exe+" -i ligand_inp.mol -o ligand_out.mol")
            for a in range(1,20):
                if not os.path.exists(out):
                    time.sleep(0.05)
                else:
                    break;
            cmd.load(out,tmp_obj,zoom=0)
            cmd.alter(self.ligand,"ID=index")
            cmd.fit(tmp_obj,self.ligand,matchmaker=2)
            cmd.update(self.ligand,tmp_obj,matchmaker=2)
            cmd.delete(tmp_obj)
            cmd.sculpt_deactivate(self.ligand)
            cmd.sculpt_purge()
            
    def set_target(self,target):
        self.target = target
        cmd.refresh_wizard()
        
    def set_ligand(self,ligand):
        self.ligand = ligand
        cmd.refresh_wizard()
        
    def get_panel(self):
        return [
            [ 1, 'Cleanup',''],
            [ 2, 'Run','cmd.get_wizard().run()'],

            [ 2, "Undo", 'cmd.get_wizard().undo()' ],
            [ 2, "Redo", 'cmd.get_wizard().redo()' ],
            [ 3, "\\999Ligand:\\000 "+self.ligand,'ligand'],
#         [ 3, "\\999Target:\\000 "+self.target,'target'],
            [ 2, 'Refresh','cmd.get_wizard().update()'],         
            [ 2, 'Done','cmd.set_wizard()'],
            ]

    def update(self):
        self.update_menus()
        cmd.refresh_wizard()
        
    def cleanup(self):
        self.clear()
        cmd.delete(undo_object)
        cmd.delete(redo_object)
        
    def clear(self):
        cmd.unpick()
        cmd.refresh_wizard()
        
    def get_prompt(self):
        self.prompt = []
        if self.ligand=="":
            self.prompt = [ 'Please pick a ligand...' ]
        if self.message!=None:
            self.prompt.append(self.message)
        return self.prompt
    
    def set_status(self,status):
        self.status = status
        cmd.refresh_wizard()

    def do_pick(self,bondFlag):
        obj = cmd.get_object_list("pkmol")
        if self.ligand=="" and len(obj)==1:
            self.ligand= obj[0]
        cmd.refresh_wizard()
