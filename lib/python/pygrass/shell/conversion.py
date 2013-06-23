# -*- coding: utf-8 -*-
"""
Created on Sun Jun 23 13:40:19 2013

@author: pietro
"""


dcont = """    <tr>
      <td>{key}</td>
      <td>{value}</td>
    </tr>"""


def dict2html(dic, keys=None, border='',
              kfmt='%s', kdec='', kfun=None,
              vfmt='%s', vdec='', vfun=None):
    """Return a html repr of a dictionary.

    Parameters
    -----------

    dic: dictionary, required
        Dictionary or object with `keys` and `items` methods
    keys: iterable, optional
        Iterable objectwith only the keys that we want to display
    border: string, optional
        Could be: "0", "1", etc.
    kfmt: string, optional
        String to format the key string (i.e. "%r", etc.)
    kdec: string, optional
        String to decorate the key (i.e. "b", "i", etc.)
    vfmt: string, optional
        String to format the value string (i.e. "%r", etc.)
    vdec: string, optional
        String to decorate the value (i.e. "b", "i", etc.)

    Examples
    ---------

    ::

        >>> dic = {'key 0': 0, 'key 1': 1}
        >>> print dict2html(dic)
        <table>
            <tr>
              <td>key 0</td>
              <td>0</td>
            </tr>
            <tr>
              <td>key 1</td>
              <td>1</td>
            </tr>
        </table>
        >>> print dict2html(dic, border="1")
        <table border='1'>
            <tr>
              <td>key 0</td>
              <td>0</td>
            </tr>
            <tr>
              <td>key 1</td>
              <td>1</td>
            </tr>
        </table>
        >>> print dict2html(dic, kdec='b', vfmt='%05d', vdec='i')
        <table>
            <tr>
              <td><b>key 0</b></td>
              <td><i>00000</i></td>
            </tr>
            <tr>
              <td><b>key 1</b></td>
              <td><i>00001</i></td>
            </tr>
        </table>
        >>> dic = {'key 0': (2, 3), 'key 1': (10, 5)}
        >>> print dict2html(dic, kdec='b', vdec='i',
        ...                 vfun=lambda x: "%d<sup>%.1f</sup>" % x)
        <table>
            <tr>
              <td><b>key 0</b></td>
              <td><i>2<sup>3.0</sup></i></td>
            </tr>
            <tr>
              <td><b>key 1</b></td>
              <td><i>10<sup>5.0</sup></i></td>
            </tr>
        </table>
    """
    def fun(x):
        return x

    keys = keys if keys else sorted(dic.keys())
    header = "<table border=%r>" % border if border else "<table>"
    kd = "<%s>%s</%s>" % (kdec, kfmt, kdec) if kdec else kfmt
    vd = "<%s>%s</%s>" % (vdec, vfmt, vdec) if vdec else vfmt
    kfun = kfun if kfun else fun
    vfun = vfun if vfun else fun
    content = [dcont.format(key=kd % kfun(k), value=vd % vfun(dic[k]))
               for k in keys]
    return '\n'.join([header, ] + content + ['</table>', ])
