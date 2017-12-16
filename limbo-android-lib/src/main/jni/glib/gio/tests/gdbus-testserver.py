#!/usr/bin/env python

import gobject
import time

import dbus
import dbus.service
import dbus.mainloop.glib

class TestException(dbus.DBusException):
    _dbus_error_name = 'com.example.TestException'


class TestService(dbus.service.Object):

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                         in_signature='s', out_signature='s')
    def HelloWorld(self, hello_message):
        if str(hello_message) == 'Yo':
            raise TestException('Yo is not a proper greeting')
        else:
            return "You greeted me with '%s'. Thanks!"%(str(hello_message))

    @dbus.service.method("com.example.Frob",
                         in_signature='ss', out_signature='ss')
    def DoubleHelloWorld(self, hello1, hello2):
        return ("You greeted me with '%s'. Thanks!"%(str(hello1)), "Yo dawg, you uttered '%s'. Thanks!"%(str(hello2)))

    @dbus.service.method("com.example.Frob",
                         in_signature='', out_signature='su')
    def PairReturn(self):
        return ("foo", 42)

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                         in_signature='ybnqiuxtdsog', out_signature='ybnqiuxtdsog')
    def TestPrimitiveTypes(self, val_byte, val_boolean, val_int16, val_uint16, val_int32, val_uint32, val_int64, val_uint64, val_double, val_string, val_objpath, val_signature):
        return val_byte + 1, not val_boolean, val_int16 + 1, val_uint16 + 1, val_int32 + 1, val_uint32 + 1, val_int64 + 1, val_uint64 + 1, -val_double + 0.123, val_string * 2, val_objpath + "/modified", val_signature * 2

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                         in_signature='ayabanaqaiauaxatad', out_signature='ayabanaqaiauaxatad')
    def TestArrayOfPrimitiveTypes(self, val_byte, val_boolean, val_int16, val_uint16, val_int32, val_uint32, val_int64, val_uint64, val_double):
        return val_byte*2, val_boolean*2, val_int16*2, val_uint16*2, val_int32*2, val_uint32*2, val_int64*2, val_uint64*2, val_double*2

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                         in_signature='asaoag', out_signature='asaoag')
    def TestArrayOfStringTypes(self, val_string, val_objpath, val_signature):
        return val_string * 2, val_objpath * 2, val_signature * 2

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                         in_signature  = 'a{yy}a{bb}a{nn}a{qq}a{ii}a{uu}a{xx}a{tt}a{dd}a{ss}a{oo}a{gg}',
                         out_signature = 'a{yy}a{bb}a{nn}a{qq}a{ii}a{uu}a{xx}a{tt}a{dd}a{ss}a{oo}a{gg}')
    def TestHashTables(self, hyy, hbb, hnn, hqq, hii, huu, hxx, htt, hdd, hss, hoo, hgg):

        ret_hyy = {}
        for i in hyy:
            ret_hyy[i*2] = (hyy[i]*3) & 255

        ret_hbb = {}
        for i in hbb:
            ret_hbb[i] = True

        ret_hnn = {}
        for i in hnn:
            ret_hnn[i*2] = hnn[i]*3

        ret_hqq = {}
        for i in hqq:
            ret_hqq[i*2] = hqq[i]*3

        ret_hii = {}
        for i in hii:
            ret_hii[i*2] = hii[i]*3

        ret_huu = {}
        for i in huu:
            ret_huu[i*2] = huu[i]*3

        ret_hxx = {}
        for i in hxx:
            ret_hxx[i + 2] = hxx[i] + 1

        ret_htt = {}
        for i in htt:
            ret_htt[i + 2] = htt[i] + 1

        ret_hdd = {}
        for i in hdd:
            ret_hdd[i + 2.5] = hdd[i] + 5.0

        ret_hss = {}
        for i in hss:
            ret_hss[i + "mod"] = hss[i]*2

        ret_hoo = {}
        for i in hoo:
            ret_hoo[i + "/mod"] = hoo[i] + "/mod2"

        ret_hgg = {}
        for i in hgg:
            ret_hgg[i + "assgit"] = hgg[i]*2

        return ret_hyy, ret_hbb, ret_hnn, ret_hqq, ret_hii, ret_huu, ret_hxx, ret_htt, ret_hdd, ret_hss, ret_hoo, ret_hgg

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                         in_signature='(ii)(s(ii)aya{ss})', out_signature='(ii)(s(ii)aya{ss})')
    def TestStructureTypes(self, s1, s2):
        (x, y) = s1;
        (desc, (x1, y1), ay, hss) = s2;
        ret_hss = {}
        for i in hss:
            ret_hss[i] = hss[i] + " ... in bed!"
        return (x + 1, y + 1), (desc + " ... in bed!", (x1 + 2, y1 + 2), ay * 2, ret_hss)

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                         in_signature='vb', out_signature='v')
    def TestVariant(self, v, modify):

        if modify:
            if type(v)==dbus.Boolean:
                ret = False
            elif type(v)==dbus.Dictionary:
                ret = {}
                for i in v:
                    ret[i] = v[i] * 2
            elif type(v)==dbus.Struct:
                ret = ["other struct", dbus.Int16(100)]
            else:
                ret = v * 2
        else:
            ret = v
        return (type(v))(ret)

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                         in_signature='a(ii)aa(ii)aasaa{ss}aayavaav', out_signature='a(ii)aa(ii)aasaa{ss}aayavaav')
    def TestComplexArrays(self, aii, aaii, aas, ahashes, aay, av, aav):
        return aii * 2, aaii * 2, aas * 2, ahashes * 2, aay * 2, av *2, aav * 2

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                          in_signature='a{s(ii)}a{sv}a{sav}a{saav}a{sa(ii)}a{sa{ss}}',
                         out_signature='a{s(ii)}a{sv}a{sav}a{saav}a{sa(ii)}a{sa{ss}}')
    def TestComplexHashTables(self, h_str_to_pair, h_str_to_variant, h_str_to_av, h_str_to_aav,
                              h_str_to_array_of_pairs, hash_of_hashes):

        ret_h_str_to_pair = {}
        for i in h_str_to_pair:
            ret_h_str_to_pair[i + "_baz"] = h_str_to_pair[i]

        ret_h_str_to_variant = {}
        for i in h_str_to_variant:
            ret_h_str_to_variant[i + "_baz"] = h_str_to_variant[i]

        return ret_h_str_to_pair, ret_h_str_to_variant, h_str_to_av, h_str_to_aav, h_str_to_array_of_pairs, hash_of_hashes

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                          in_signature='', out_signature='')
    def Quit(self):
        mainloop.quit()

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                          in_signature='sv', out_signature='')
    def FrobSetProperty(self, prop_name, prop_value):
        self.frob_props[prop_name] = prop_value
        message = dbus.lowlevel.SignalMessage("/com/example/TestObject",
                                              "org.freedesktop.DBus.Properties",
                                              "PropertiesChanged")
        message.append("com.example.Frob")
        message.append({prop_name : prop_value})
        message.append([], signature="as")
        session_bus.send_message(message)

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob",
                          in_signature='', out_signature='')
    def FrobInvalidateProperty(self):
        self.frob_props["PropertyThatWillBeInvalidated"] = "OMGInvalidated"
        message = dbus.lowlevel.SignalMessage("/com/example/TestObject",
                                              "org.freedesktop.DBus.Properties",
                                              "PropertiesChanged")
        message.append("com.example.Frob")
        message.append({}, signature="a{sv}")
        message.append(["PropertyThatWillBeInvalidated"])
        session_bus.send_message(message)

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.signal("com.example.Frob",
                         signature="sov")
    def TestSignal(self, str1, objpath1, variant1):
        pass

    @dbus.service.method("com.example.Frob",
                          in_signature='so', out_signature='')
    def EmitSignal(self, str1, objpath1):
        self.TestSignal (str1 + " .. in bed!", objpath1 + "/in/bed", "a variant")

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("com.example.Frob", in_signature='i', out_signature='',
                         async_callbacks=('return_cb', 'raise_cb'))
    def Sleep(self, msec, return_cb, raise_cb):
        def return_from_async_wait():
            return_cb()
            return False
        gobject.timeout_add(msec, return_from_async_wait)

    # ----------------------------------------------------------------------------------------------------

    @dbus.service.method("org.freedesktop.DBus.Properties",
                         in_signature  = 'ss',
                         out_signature = 'v')
    def Get(self, interface_name, property_name):

        if interface_name == "com.example.Frob":
            return self.frob_props[property_name]
        else:
            raise TestException("No such interface " + interface_name)

    @dbus.service.method("org.freedesktop.DBus.Properties",
                         in_signature  = 's',
                         out_signature = 'a{sv}')
    def GetAll(self, interface_name):
        if interface_name == "com.example.Frob":
            return self.frob_props
        else:
            raise TestException("No such interface " + interface_name)

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    session_bus = dbus.SessionBus()
    name = dbus.service.BusName("com.example.TestService", session_bus)

    obj = TestService(session_bus, '/com/example/TestObject')

    #print "Our unique name is %s"%(session_bus.get_unique_name())

    obj.frob_props = {}
    obj.frob_props["y"] = dbus.Byte(1)
    obj.frob_props["b"] = dbus.Boolean(True)
    obj.frob_props["n"] = dbus.Int16(2)
    obj.frob_props["q"] = dbus.UInt16(3)
    obj.frob_props["i"] = dbus.Int32(4)
    obj.frob_props["u"] = dbus.UInt32(5)
    obj.frob_props["x"] = dbus.Int64(6)
    obj.frob_props["t"] = dbus.UInt64(7)
    obj.frob_props["d"] = dbus.Double(7.5)
    obj.frob_props["s"] = dbus.String("a string")
    obj.frob_props["o"] = dbus.ObjectPath("/some/path")
    obj.frob_props["ay"] = [dbus.Byte(1), dbus.Byte(11)]
    obj.frob_props["ab"] = [dbus.Boolean(True), dbus.Boolean(False)]
    obj.frob_props["an"] = [dbus.Int16(2), dbus.Int16(12)]
    obj.frob_props["aq"] = [dbus.UInt16(3), dbus.UInt16(13)]
    obj.frob_props["ai"] = [dbus.Int32(4), dbus.Int32(14)]
    obj.frob_props["au"] = [dbus.UInt32(5), dbus.UInt32(15)]
    obj.frob_props["ax"] = [dbus.Int64(6), dbus.Int64(16)]
    obj.frob_props["at"] = [dbus.UInt64(7), dbus.UInt64(17)]
    obj.frob_props["ad"] = [dbus.Double(7.5), dbus.Double(17.5)]
    obj.frob_props["as"] = [dbus.String("a string"), dbus.String("another string")]
    obj.frob_props["ao"] = [dbus.ObjectPath("/some/path"), dbus.ObjectPath("/another/path")]
    obj.frob_props["foo"] = "a frobbed string"
    obj.frob_props["PropertyThatWillBeInvalidated"] = "InitialValue"

    mainloop = gobject.MainLoop()
    mainloop.run()
