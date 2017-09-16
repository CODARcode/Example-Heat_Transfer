/* 
 * Staged write of ADIOS files using a staging method
 *
 * Copyright (c) 2008 - 2012.  UT-BATTELLE, LLC. All rights reserved.
 */


/* Staged write example code.
   Assumptions:
     - one output step fits into the memory of the staged writer.
       Actually, this means, even more memory is needed than the size of output.
       We need to read each variable while also buffering all of them for output.
     - output steps contain the same variable set (no changes in variables)
     - attributes are the same for all steps (will write only once here)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "mpi.h"
#include "utils.h"
#include "adios.h"
#include "adios_read.h"
#include "adios_error.h"
#include "decompose.h"

// Input arguments
char   infilename[256];    // File/stream to read 
char   outfilename[256];   // File to write
char   wmethodname[16];     // ADIOS write method
char   wmethodparams[256];  // ADIOS write method
char   rmethodname[16];     // ADIOS read method
char   rmethodparams[256];  // ADIOS read method
char   varnames[256];     // ADIOS variable names
char   transparams[256];  // ADIOS transform params
enum ADIOS_READ_METHOD read_method;

static const int max_read_buffer_size  = 1024*1024*1024;
static const int max_write_buffer_size = 1024*1024*1024;

static int timeout_sec = 10; // will stop if no data found for this time (-1: never stop)

typedef struct {
    ADIOS_VARINFO * v;
    uint64_t        start[10];
    uint64_t        count[10];
    uint64_t        writesize; // size of subset this process writes, 0: do not write
} VarInfo;

VarInfo * varinfo;

// Global variables
int         rank, numproc;
MPI_Comm    comm; 
ADIOS_FILE *f;      // stream for reading
int64_t    fh;     // ADIOS output file handle
int64_t     gh;     // ADIOS group for output definitions
uint64_t    write_total; // data size written by one processor
uint64_t    largest_block; // the largest variable block one process reads
char       *group_name; // name of ADIOS group
char       *readbuf; // read buffer
int         decomp_values[10];


int process_metadata(int step);
int read_write(int step, double*);

void printUsage(char *prgname)
{
    print0("Usage: %s input output rmethod \"params\" wmethod \"params\" [names params <decomposition>]\n"
           "    input   Input stream path\n"
           "    output  Output file path\n"
           "    rmethod ADIOS method to read with\n"
           "            Supported read methods: BP, DATASPACES, DIMES, FLEXPATH\n"
           "    params  Read method parameters (in quotes; comma-separated list)\n"
           "    wmethod ADIOS method to write with\n"
           "    params  Write method parameters (in quotes; comma-separated list)\n"
           "    names   List of variables to apply transforms(compression) (in quotes; comma-separated list)\n"
           "    params  Transform parameters (in quotes)\n"
           "    <decomposition>    list of numbers e.g. 32 8 4\n"
           "            Decomposition values in each dimension of an array\n"
           "            The product of these number must be less then the number\n"
           "            of processes. Processes whose rank is higher than the\n"
           "            product, will not write anything.\n"
           "               Arrays with less dimensions than the number of values,\n"
           "            will be decomposed with using the appropriate number of\n"
           "            values.\n"
        ,prgname);
}


int processArgs(int argc, char ** argv)
{
    int i, j, nd, prod;
    char *end;
    if (argc < 7) {
        printUsage (argv[0]);
        return 1;
    }
    strncpy(infilename,     argv[1], sizeof(infilename));
    strncpy(outfilename,    argv[2], sizeof(outfilename));
    strncpy(rmethodname,    argv[3], sizeof(rmethodname));
    strncpy(rmethodparams,  argv[4], sizeof(rmethodparams));
    strncpy(wmethodname,    argv[5], sizeof(wmethodname));
    strncpy(wmethodparams,  argv[6], sizeof(wmethodparams));
    if (argc>7) strncpy(varnames,       argv[7], sizeof(varnames));
    if (argc>8) strncpy(transparams,    argv[8], sizeof(transparams));
    
    nd = 0;
    j = 9;
    while (argc > j && j<15) { // get max 6 dimensions
        errno = 0; 
        decomp_values[nd] = strtol(argv[j], &end, 10); 
        if (errno || (end != 0 && *end != '\0')) { 
            print0 ("ERROR: Invalid decomposition number in argument %d: '%s'\n",
                    j, argv[j]); 
            printUsage(argv[0]);
            return 1; 
        } 
        nd++; 
        j++;
    }

    if (argc > j) { 
        print0 ("ERROR: Only 6 decompositon arguments are supported\n");
        return 1; 
    } 

    // Set default value
    if (nd == 0) {
        decomp_values[0] = numproc;
        nd = 1;
    }

    for (i=nd; i<10; i++) {
        decomp_values[i] = 1;
    }

    prod = 1;
    for (i=0; i<nd; i++) {
        prod *= decomp_values[i];
    }

    if (prod > numproc) {
        print0 ("ERROR: Product of decomposition numbers %d > number of processes %d\n", 
                prod, numproc);
        printUsage(argv[0]);
        return 1; 
    }

    if (!strcmp(rmethodname,"BP")) {
        read_method = ADIOS_READ_METHOD_BP;
    } else if (!strcmp(rmethodname,"DATASPACES")) {
        read_method = ADIOS_READ_METHOD_DATASPACES;
    } else if (!strcmp(rmethodname,"DIMES")) {
        read_method = ADIOS_READ_METHOD_DIMES;
    } else if (!strcmp(rmethodname,"FLEXPATH")) {
        read_method = ADIOS_READ_METHOD_FLEXPATH;
    } else {
        print0 ("ERROR: Supported read methods are: BP, DATASPACES, DIMES, FLEXPATH. You selected %s\n", rmethodname);
    }
    
    if (!strcmp(rmethodparams,"")) {
        strcpy (rmethodparams, "max_chunk_size=100; "
                               "app_id =32767; \n"
                               "verbose= 3;"
                               "poll_interval  =  100;");
    }

    return 0;
}


int main (int argc, char ** argv) 
{
    int         err;
    int         steps = 0, curr_step;
    int         retval = 0;

    double      tick, tock;
    double      io_time = 0.0;
    double      this_step_timestamp, prev_step_timestamp;
    double      t1, t2, write_time, total_write_time;

    MPI_Init (&argc, &argv);
    //comm = MPI_COMM_WORLD;
    //MPI_Comm_rank (comm, &rank);
    //MPI_Comm_size (comm, &numproc);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &numproc);
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Comm_split(MPI_COMM_WORLD, 2, rank, &comm);	//color=2
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &numproc);

    if (rank == 0) tick = MPI_Wtime();

    if (processArgs(argc, argv)) {
        return 1;
    }
    
    print0("Input stream            = %s\n", infilename);
    print0("Output stream           = %s\n", outfilename);
    print0("Read method             = %s (id=%d)\n", rmethodname, read_method);
    print0("Read method parameters  = \"%s\"\n", rmethodparams);
    print0("Write method            = %s\n", wmethodname);
    print0("Write method parameters = \"%s\"\n", wmethodparams);
    print0("Variable to transform   = \"%s\"\n", varnames);
    print0("Transform parameters    = \"%s\"\n", transparams);
    

    err = adios_read_init_method(read_method, comm, rmethodparams);

    if (!err) {
        print0 ("%s\n", adios_errmsg());
    }

    adios_init_noxml(comm);

    print0 ("Waiting to open stream %s...\n", infilename);
    f = adios_read_open_stream (infilename, read_method, comm, 
                                             ADIOS_LOCKMODE_ALL, timeout_sec);
    if (adios_errno == err_file_not_found) 
    {
        print ("rank %d: Stream not found after waiting %d seconds: %s\n", 
               rank, timeout_sec, adios_errmsg());
        retval = adios_errno;
    } 
    else if (adios_errno == err_end_of_stream) 
    {
        print ("rank %d: Stream terminated before open. %s\n", rank, adios_errmsg());
        retval = adios_errno;
    } 
    else if (f == NULL) {
        print ("rank %d: Error at opening stream: %s\n", rank, adios_errmsg());
        retval = adios_errno;
    } 
    else 
    {
        // process steps here... 
        if (f->current_step != 0) {
            print ("rank %d: WARNING: First %d steps were missed by open.\n", 
                   rank, f->current_step);
        }

        prev_step_timestamp = MPI_Wtime();
        while (1)
        {
            steps++; // start counting from 1

            print0 ("File info:\n");
            print0 ("  current step:   %d\n", f->current_step);
            print0 ("  last step:      %d\n", f->last_step);
            print0 ("  # of variables: %d:\n", f->nvars);

            if (rank == 0) {
                this_step_timestamp = MPI_Wtime();
                printf("step gap: %lf\n", this_step_timestamp - prev_step_timestamp);
                prev_step_timestamp = this_step_timestamp;
            }

            t1 = MPI_Wtime();
            retval = process_metadata(steps);
            if (retval) break;
            t2 = MPI_Wtime();
            if(rank==0) printf("stage_write rank %d time to process metadata %lf\n", rank, t2-t1);

            t1 = MPI_Wtime();
            retval = read_write(steps, &write_time);
            if (retval) break;
            t2 = MPI_Wtime();
            io_time += t2-t1;
            if(rank==0) printf("stage_write rank %d time to read write %lf\n", rank, t2-t1);

            // advance to 1) next available step with 2) blocking wait 
            curr_step = f->current_step; // save for final bye print
            t1 = MPI_Wtime();
            adios_advance_step (f, 0, timeout_sec);
            t2 = MPI_Wtime();
            if(rank==0) printf("stage_write rank %d time to advance step %lf\n", rank, t2-t1);

            if (adios_errno == err_end_of_stream) 
            {
                break; // quit while loop
            }
            else if (adios_errno == err_step_notready) 
            {
                print ("rank %d: No new step arrived within the timeout. Quit. %s\n", 
                        rank, adios_errmsg());
                break; // quit while loop
            } 
            else if (f->current_step != curr_step+1) 
            {
                // we missed some steps
                print ("rank %d: WARNING: steps %d..%d were missed when advancing.\n", 
                        rank, curr_step+1, f->current_step-1);
            }

        }

        adios_read_close (f);
        if(readbuf) free(readbuf);
        if(varinfo) free(varinfo);
        if(group_name) free(group_name);
    } 
    print0 ("Bye after processing %d steps\n", steps);

    adios_read_finalize_method (read_method);
    adios_finalize (rank);

    if (rank == 0) tock = MPI_Wtime();
    MPI_Reduce(&write_time, &total_write_time, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    print0("Stage_write runtime: %lf\nStage_write io time: %lf\nStage_write write time: %lf\n", tock-tick, io_time, total_write_time);

    MPI_Finalize ();

    return retval;
}


int process_metadata(int step)
{
    int retval = 0;
    int i, j;
    char gdims[256], ldims[256], offs[256];
    uint64_t sum_count;
    ADIOS_VARINFO *v; // shortcut pointer
    char     ** group_namelist; // name(s) of ADIOS group(s)


    if (step > 1)
    {
        // right now, nothing to prepare in later steps
        // print("Step %d. return immediately\n",step);
        return 0;
    }

    /* First step processing */

    // Get group name, then declare the group for output
    adios_get_grouplist(f, &group_namelist);
    group_name = strdup (group_namelist[0]);
    print0("Group name is %s\n", group_name);
    adios_declare_group(&gh,group_name,"",adios_stat_full);


    varinfo = (VarInfo *) malloc (sizeof(VarInfo) * f->nvars);
    if (!varinfo) {
        print("ERROR: rank %d cannot allocate %" PRIu64 " bytes\n", 
                rank, (uint64_t)(sizeof(VarInfo)*f->nvars));
        return 1;
    }

    write_total = 0;
    largest_block = 0;

    // Decompose each variable and calculate output buffer size
    for (i=0; i<f->nvars; i++) 
    {
        print0 ("Get info on variable %d: %s\n", i, f->var_namelist[i]); 
        varinfo[i].v = adios_inq_var_byid (f, i);
        v = varinfo[i].v; // just a shortcut
        if (v == NULL) {
            print ("rank %d: ERROR: Variable %s inquiry failed: %s\n", 
                   rank, f->var_namelist[i], adios_errmsg());
            return 1;
        }

        // print variable type and dimensions
        print0("    %-9s  %s", adios_type_to_string(v->type), f->var_namelist[i]);
        if (v->ndim > 0) {
            print0("[%" PRIu64, v->dims[0]);
            for (j = 1; j < v->ndim; j++)
                print0(", %" PRIu64, v->dims[j]);
            print0("] :\n");
        } else {
            print0("\tscalar\n");
        }

        // determine subset we will write
        decompose (numproc, rank, v->ndim, v->dims, decomp_values,
                   varinfo[i].count, varinfo[i].start, &sum_count);
        varinfo[i].writesize = sum_count * adios_type_size(v->type, v->value);

        if (varinfo[i].writesize != 0) {
            write_total += varinfo[i].writesize;
            if (largest_block < varinfo[i].writesize)
                largest_block = varinfo[i].writesize; 
        }

    }

    // determine output buffer size and allocate it
    uint64_t bufsize = write_total + f->nvars*128 + f->nattrs*32 + 1024; 
    if (bufsize > max_write_buffer_size) {
        print ("ERROR: rank %d: write buffer size needs to hold about %" PRIu64 " bytes, "
                "but max is set to %d\n", rank, bufsize, max_write_buffer_size);
        return 1;
    }
    print0 ("Rank %d: allocate %" PRIu64 " MB for output buffer\n", rank, bufsize/1048576+1);
    adios_set_max_buffer_size (bufsize/1048576+1); 

    // allocate read buffer
    bufsize = largest_block + 128;
    if (bufsize > max_read_buffer_size) {
        print ("ERROR: rank %d: read buffer size needs to hold at least %" PRIu64 " bytes, "
                "but max is set to %d\n", rank, bufsize, max_read_buffer_size);
        return 1;
    }
    print0 ("Rank %d: allocate %g MB for input buffer\n", rank, (double)bufsize/1048576.0);
    readbuf = (char *) malloc ((size_t)bufsize);
    if (!readbuf) {
        print ("ERROR: rank %d: cannot allocate %" PRIu64 " bytes for read buffer\n",
               rank, bufsize);
        return 1;
    }

    // Select output method
    adios_select_method (gh, wmethodname, wmethodparams, "");

	// TAHSIN
	//adios_set_time_aggregation(gh,64*1024*1024,0);

    // Define variables for output based on decomposition
    char *vpath, *vname;
    for (i=0; i<f->nvars; i++) 
    {
        v = varinfo[i].v;
        if (varinfo[i].writesize != 0) {
            // define variable for ADIOS writes
            getbasename (f->var_namelist[i], &vpath, &vname);

            if (v->ndim > 0) 
            {
                int64s_to_str (v->ndim, v->dims, gdims);
                int64s_to_str (v->ndim, varinfo[i].count, ldims);
                int64s_to_str (v->ndim, varinfo[i].start, offs);

		/*
                print ("rank %d: Define variable path=\"%s\" name=\"%s\"  "
                       "gdims=%s  ldims=%s  offs=%s\n", 
                       rank, vpath, vname, gdims, ldims, offs);
		*/

                int64_t var_id;
                var_id = adios_define_var (gh, vname, vpath, v->type, ldims, gdims, offs);

                char *varnames_ = strdup(varnames);
                char* token = strtok (varnames_, ", ");
                while (token) 
                {
                    if (!strcmp (vname, token))
                    {
                        print ("rank %d: Set transform: %s\n", rank, token);
                        adios_set_transform (var_id, transparams);
                    }
                    token = strtok (NULL, ", ");
                }
                free(varnames_);
            }
            else 
            {
		/*
                print ("rank %d: Define scalar path=\"%s\" name=\"%s\"\n",
                       rank, vpath, vname); */

                adios_define_var (gh, vname, vpath, v->type, "", "", "");
            }
            free(vpath);
            free(vname);
        }
    }

    if (rank == 0)
    {
        // get and define attributes
        enum ADIOS_DATATYPES attr_type;
        void * attr_value;
        char * attr_value_str;
        int  attr_size;
        for (i=0; i<f->nattrs; i++) 
        {
            adios_get_attr_byid (f, i, &attr_type, &attr_size, &attr_value);
            attr_value_str = (char *)value_to_string (attr_type, attr_value, 0);
            getbasename (f->attr_namelist[i], &vpath, &vname);
            if (vpath && !strcmp(vpath,"/__adios__")) { 
                // skip on /__adios/... attributes 
                print ("rank %d: Ignore this attribute path=\"%s\" name=\"%s\" value=\"%s\"\n",
                        rank, vpath, vname, attr_value_str);
            } else {
                adios_define_attribute (gh, vname, vpath,
                        attr_type, attr_value_str, "");
                print ("rank %d: Define attribute path=\"%s\" name=\"%s\" value=\"%s\"\n",
                        rank, vpath, vname, attr_value_str);
                free (attr_value);
            }
        }
    }

    return retval;
}

#define MAX_TIMESTEPS_PER_FILE (3*1024)
int  time_step_count = 0;
int  current_idx = 0;
char currentfile[256];

int read_write(int step, double *write_time)
{
    int retval = 0;
    int i;
    uint64_t total_size;
    double t1, t2;

    sprintf(currentfile,"%s%d",outfilename,current_idx);

    // open output file
    adios_open (&fh, group_name, currentfile, (time_step_count==0 ? "w" : "a"), comm);
    adios_group_size (fh, write_total, &total_size);
    
    for (i=0; i<f->nvars; i++) 
    {
        if (varinfo[i].writesize != 0) {
            // read variable subset
            // print ("rank %d: Read variable %d: %s\n", rank, i, f->var_namelist[i]); 
            ADIOS_SELECTION *sel = adios_selection_boundingbox (varinfo[i].v->ndim,
                    varinfo[i].start, 
                    varinfo[i].count);
            adios_schedule_read_byid (f, sel, i, 0, 1, readbuf);
            adios_perform_reads (f, 1);   


            // write (buffer) variable
            // print ("rank %d: Write variable %d: %s\n", rank, i, f->var_namelist[i]); 
            adios_write(fh, f->var_namelist[i], readbuf);
        }
    }

    adios_release_step (f); // this step is no longer needed to be locked in staging area
    t1 = MPI_Wtime();
    adios_close (fh); // write out output buffer to file
    t2 = MPI_Wtime();

    *write_time = *write_time + t2-t1;

    if ((++time_step_count)>=MAX_TIMESTEPS_PER_FILE) {
		current_idx++;
		time_step_count = 0;
    }

    return retval;
}

