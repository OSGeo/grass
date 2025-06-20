def __getattr__(name):
    if name == "Tools":
        from .session_tools import Tools

        return Tools
    msg = f"module {__name__} has no attribute {name}"
    raise AttributeError(msg)


__all__ = ["Tools"]  # pylint: disable=undefined-all-variable
