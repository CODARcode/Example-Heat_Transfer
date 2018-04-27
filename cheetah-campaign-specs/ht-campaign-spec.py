from codar.cheetah import Campaign
from codar.cheetah import parameters as p
from codar.cheetah.parameters import SymLink

class HeatTransfer(Campaign):

    name = "heat-transfer"

    codes = [ 
              ("dataspaces", dict(exe="/lustre/atlas/proj-shared/csc249/CSC249ADCD01/software/titan.gnu/gcc-4.9.3/dataspaces-develop/bin/dataspaces_server",
                                  sleep_after=10,
                                  linked_with_sosflow=False)),
              ("stage_write", dict(exe="stage_write/stage_write_tau", sleep_after=5,
                                   linked_with_sosflow=True)),
              ("heat", dict(exe="heat_transfer_adios2_tau", sleep_after=5,
                            linked_with_sosflow=True,
                            adios_xml_file='heat_transfer.xml')),
            ]

    supported_machines = ['local','titan']
    kill_on_partial_failure=True
    sosd_path = "/lustre/atlas/proj-shared/csc249/CSC249ADCD01/software/titan.gnu/gcc-4.9.3/sos_flow/bin/sosd"
    run_post_process_script = "run-post-process.sh"
    umask = '027'
    
    scheduler_options = {
        "titan": {"project":"CSC249ADCD01",
                  "queue":"batch" }
    }
    
    app_config_scripts = {
        'titan': 'titan_config.sh',
        'local': None,
    }

    sweeps = [
     
     p.SweepGroup(
      "sim",
      nodes=384,
      walltime=2500,
      per_run_timeout=1000,
      sosflow_profiling=True,
      component_subdirs=False,
      #component_inputs={"heat":[SymLink("workflow.swift"), SymLink(
      #    "stage_write"), "dataspaces.conf"]},

      parameter_groups=
       [p.Sweep(
        node_layout={ "titan": [{ "heat": 16}] },
        parameters=[
        
        p.ParamRunner("heat", "nprocs", [2048]),
        p.ParamCmdLineArg("heat", "output", 1, ["heat_sim_data"]),
        p.ParamCmdLineArg("heat", "xprocs", 2, [64]),
        p.ParamCmdLineArg("heat", "yprocs", 3, [32]),
        p.ParamCmdLineArg("heat", "xsize", 4, [4096]),
        p.ParamCmdLineArg("heat", "ysize", 5, [3072]),
        p.ParamCmdLineArg("heat", "timesteps", 6, [1000]),
        p.ParamCmdLineArg("heat", "checkpoints", 7, [4]),
        
        p.ParamAdiosXML("heat", "transform_T", "adios_transform:heat:T",
            ["none", "zlib:5", "bzip2:5", "sz:absolute=0.001", "zfp:accuracy=0.001", "mgard:tol=0.001", "blosc:threshold=4096,shuffle=bit,lvl=1,threads=4,compressor=zstd"]),
        p.ParamAdiosXML("heat", "transport_T", "adios_transport:heat",
            ["MPI_AGGREGATE:num_aggregators=128;num_ost=128;have_metadata_file=0;"]),
        p.ParamAdiosXML("heat", "transport_T_final", "adios_transport:heat_final",
            ["MPI_AGGREGATE:num_aggregators=128;num_ost=128;have_metadata_file=0;"]),
        ]),
      ]),

     
     p.SweepGroup(
      "staging-1_ppn",
      nodes=520,
      walltime=7500,
      per_run_timeout=2500,
      sosflow_profiling=True,
      #component_inputs={"dataspaces":["dataspaces.conf"],},
     
      parameter_groups=
      [p.Sweep(
        node_layout={ "titan": [{ "dataspaces": 1 }, {"stage_write": 1}, {"heat": 16}] },
        parameters=[
     
     
        p.ParamRunner("stage_write", "nprocs", [64,128]),
        p.ParamCmdLineArg("stage_write", "input", 1, ["heat_sim_data.bp"]),
        p.ParamCmdLineArg("stage_write", "output", 2, ["staging_output.bp"]),
        p.ParamCmdLineArg("stage_write", "rmethod", 3, ["DIMES"]),
        p.ParamCmdLineArg("stage_write", "ropt", 4, [""]),
        p.ParamCmdLineArg("stage_write", "wmethod", 5, ["POSIX"]),
        p.ParamCmdLineArg("stage_write", "wopt", 6, ["have_metadata_file=0;"]),
        p.ParamCmdLineArg("stage_write", "variables", 7, ["T"]),
        p.ParamCmdLineArg("stage_write", "transform", 8,
                          ["none","zfp:accuracy=0.001","sz:absolute=0.001","zlib:5","bzip2:5"]),
     
        p.ParamRunner("heat", "nprocs", [2048]),
        p.ParamCmdLineArg("heat", "output", 1, ["heat_sim_data"]),
        p.ParamCmdLineArg("heat", "xprocs", 2, [64]),
        p.ParamCmdLineArg("heat", "yprocs", 3, [32]),
        p.ParamCmdLineArg("heat", "xsize", 4, [4096]),
        p.ParamCmdLineArg("heat", "ysize", 5, [3072]),
        p.ParamCmdLineArg("heat", "timesteps", 6, [1000]),
        p.ParamCmdLineArg("heat", "checkpoints", 7, [4]),
        
        p.ParamAdiosXML("heat", "transform_T", "adios_transform:heat:T",
            ["none",]),
        p.ParamAdiosXML("heat", "transport_T", "adios_transport:heat",
            ["DIMES"]),
        p.ParamAdiosXML("heat", "transport_T_final", "adios_transport:heat_final",
            ["MPI_AGGREGATE:num_aggregators=128;num_ost=128;have_metadata_file=0;"]),
        ]),
      ]),
     
     p.SweepGroup(
      "staging-flexpath_1_ppn",
      nodes=520,
      walltime=7500,
      per_run_timeout=2500,
      sosflow_profiling=True,
      #component_inputs={"dataspaces":["dataspaces.conf"],},
     
      parameter_groups=
      [p.Sweep(
        node_layout={ "titan": [{ "dataspaces": 1 }, {"stage_write": 1}, {"heat": 16}] },
        parameters=[
     
     
        p.ParamRunner("stage_write", "nprocs", [64,128]),
        p.ParamCmdLineArg("stage_write", "input", 1, ["heat_sim_data.bp"]),
        p.ParamCmdLineArg("stage_write", "output", 2, ["staging_output.bp"]),
        p.ParamCmdLineArg("stage_write", "rmethod", 3, ["FLEXPATH"]),
        p.ParamCmdLineArg("stage_write", "ropt", 4, [""]),
        p.ParamCmdLineArg("stage_write", "wmethod", 5, ["POSIX"]),
        p.ParamCmdLineArg("stage_write", "wopt", 6, ["have_metadata_file=0;"]),
        p.ParamCmdLineArg("stage_write", "variables", 7, ["T"]),
        p.ParamCmdLineArg("stage_write", "transform", 8,
                          ["none","zfp:accuracy=0.001","sz:absolute=0.001","zlib:5","bzip2:5","blosc:threshold=4096,shuffle=bit,lvl=1,threads=4,compressor=zstd"]),
     
        p.ParamRunner("heat", "nprocs", [2048]),
        p.ParamCmdLineArg("heat", "output", 1, ["heat_sim_data"]),
        p.ParamCmdLineArg("heat", "xprocs", 2, [64]),
        p.ParamCmdLineArg("heat", "yprocs", 3, [32]),
        p.ParamCmdLineArg("heat", "xsize", 4, [4096]),
        p.ParamCmdLineArg("heat", "ysize", 5, [3072]),
        p.ParamCmdLineArg("heat", "timesteps", 6, [1000]),
        p.ParamCmdLineArg("heat", "checkpoints", 7, [4]),
        
        p.ParamAdiosXML("heat", "transform_T", "adios_transform:heat:T",
            ["none",]),
        p.ParamAdiosXML("heat", "transport_T", "adios_transport:heat",
            ["FLEXPATH"]),
        p.ParamAdiosXML("heat", "transport_T_final", "adios_transport:heat_final",
            ["MPI_AGGREGATE:num_aggregators=128;num_ost=128;have_metadata_file=0;"]),
        ]),
      ]),
     
    ]

