#
# This example illustrates the format of a Cheetah configuration file
#
from codar.cheetah import Campaign
from codar.cheetah import parameters as p

class HeatTransfer(Campaign):
    """Small example to run the heat_transfer application with stage_write,
    using no compression, zfp, or sz. All other options are fixed, so there
    are only three runs."""

    name = "heat-transfer"

    codes = dict(heat="heat_transfer_adios2",
                 stage="stage_write/stage_write")

    supported_machines = ['titan_fob']

    inputs = ["heat_transfer.xml"]

    project = "CSC143"
    queue = "batch"

    sweeps = [

     p.SweepGroup(nodes=64,

      post_processing = "",

      parameter_groups=
      [#p.Sweep([

       # p.ParamRunner("stage", "nprocs", [64]),

       # p.ParamCmdLineArg("stage", "input", 1, ["heat.bp"]),
       # p.ParamCmdLineArg("stage", "output", 2, ["staged.bp"]),
       # p.ParamCmdLineArg("stage", "rmethod", 3, ["FLEXPATH"]),
       # p.ParamCmdLineArg("stage", "ropt", 4, [""]),
       # p.ParamCmdLineArg("stage", "wmethod", 5, ["POSIX"]),
       # p.ParamCmdLineArg("stage", "wopt", 6, ["have_metadata_file=0"]),
       # p.ParamCmdLineArg("stage", "variables", 7, ["T,dT"]),
       # p.ParamCmdLineArg("stage", "transform", 8,
       #                   ["none","zfp:accuracy=0.001","lz4"]),

       # p.ParamRunner("heat", "nprocs", [1024]),
       # p.ParamCmdLineArg("heat", "output", 1, ["heat"]),
       # p.ParamCmdLineArg("heat", "xprocs", 2, [32]),
       # p.ParamCmdLineArg("heat", "yprocs", 3, [32]),
       # p.ParamCmdLineArg("heat", "xsize", 4, [100]),
       # p.ParamCmdLineArg("heat", "ysize", 5, [100]),
       # p.ParamCmdLineArg("heat", "steps", 6, [100]),
       # p.ParamCmdLineArg("heat", "iterations", 7, [1]),
       # ]),
        p.Sweep([

        p.ParamRunner("heat", "nprocs", [1024,1024,1024]),
        p.ParamCmdLineArg("heat", "output", 1, ["heat"]),
        p.ParamCmdLineArg("heat", "xprocs", 2, [32]),
        p.ParamCmdLineArg("heat", "yprocs", 3, [32]),
        p.ParamCmdLineArg("heat", "xsize", 4, [100]),
        p.ParamCmdLineArg("heat", "ysize", 5, [100]),
        p.ParamCmdLineArg("heat", "steps", 6, [100]),
        p.ParamCmdLineArg("heat", "iterations", 7, [1]),
        ]),
      ]),
    ]
