import io;
import launch;
import string;

app(void signal) check_conf_exists () {
       "./check_conf_exists.sh"
}

program3 = "dataspaces_server";
arguments3 = split("-s 1 -c 15", " ");
printf("swift: launching %s", program3);
exit_code3 = @par=1 launch(program3, arguments3);
printf("swift: received exit code: %d", exit_code3);
if(exit_code3 != 0)
{
    printf("swift: The launched application did not succed.");
}

check_conf_exists() => {
    program1 = "./heat_transfer_adios2";
    arguments1 = split("heat  4 3  40 50 10 50", " ");
    printf("swift: launching: %s", program1);
    exit_code1 = @par=12 launch(program1, arguments1);
    printf("swift: received exit code: %d", exit_code1);
    if (exit_code1 != 0)
    {
        printf("swift: The launched application did not succeed.");
    }

    program2 = "stage_write/stage_write";
    arguments2 = split("heat.bp staged.bp DATASPACES \"\" MPI \"\" 3 1 1", " ");
    printf("size: %i", size(arguments2));
    printf("swift: launching: %s", program2);
    exit_code2 = @par=3 launch(program2, arguments2);
    printf("swift: received exit code: %d", exit_code2);
    if (exit_code2 != 0)
    {
        printf("swift: The launched application did not succeed.");
    }
}
