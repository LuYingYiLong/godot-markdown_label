#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "addons/markdown_label/bin/libmarkdown_label.{}.{}.framework/libmarkdown_label.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "addons/markdown_label/bin/libmarkdown_label.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "addons/markdown_label/bin/libmarkdown_label.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    if env["platform"] == "windows" and not env["use_mingw"]:
        env.Append(CCFLAGS=["/utf-8", "/wd4828"])
        env.Append(CXXFLAGS=["/utf-8"])

    library = env.SharedLibrary(
        "addons/markdown_label/bin/markdown_label{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
