
import os, sys, shutil, glob
os.chdir(os.path.split(os.path.realpath(__file__))[0])

input_filename = sys.argv[1]
nb_boostraps = int(sys.argv[2])
output_dir_temp = os.path.join(sys.argv[3], "__temp__")
PERCENTS = [1, 2, 3, 4, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90]




simka_out_dir = os.path.join(output_dir_temp, "simka_results")
simka_command = "../../build/bin/simka "
simka_command += " -in " + input_filename
simka_command += " -out-tmp " + output_dir_temp
simka_command += " -out " + simka_out_dir

boostrap_results_dir = os.path.join(output_dir_temp, "boostrap_results")
r_input_dir = os.path.join(output_dir_temp, "r_input_dir")
r_result_dir = os.path.join(output_dir_temp, "result_figures")

class ComputeBootstraps():

    def execute(self):

        if os.path.exists(output_dir_temp):
            shutil.rmtree(output_dir_temp)
        os.makedirs(output_dir_temp)
        os.makedirs(boostrap_results_dir)

        self.setup()
        self.compute_truth()
        self.subsample()

    def setup(self):
        filename = os.path.join(output_dir_temp, "simka_subsampling_setup.txt")
        command = "../../build/bin/simka -in " + input_filename + " -out-tmp " + output_dir_temp + " -subsampling-setup"
        command += " > " + filename
        os.system(command)

        for line in open(filename, "r"):
            if "Subsampling max:" in line:
                self.subsampling_kmer_space = int(line.strip().replace("Subsampling max: ", ""))
                break

        print("Subsampling space: " + str(self.subsampling_kmer_space))

    def compute_truth(self):

        command = simka_command
        command += " -subsampling-space " + str(self.subsampling_kmer_space)
        command += " -subsampling-nb-reads " + str(self.subsampling_kmer_space)

        output_dir = os.path.join(output_dir_temp, "truth_results")

        os.system(command + " > " + os.path.join(output_dir, "log.txt"))
        shutil.move(simka_out_dir, output_dir)

    def subsample(self):

        for percent in PERCENTS:

            nb_kmers_picked = int((self.subsampling_kmer_space * percent) / float(100))

            command = simka_command
            command += " -subsampling-space " + str(self.subsampling_kmer_space)
            command += " -subsampling-nb-reads " + str(nb_kmers_picked)

            for i in range(0, nb_boostraps):
                boostrap_out_dir = os.path.join(boostrap_results_dir, "pass_" + str(percent) + "_" + str(i))

                os.system(command + " > " + os.path.join(boostrap_out_dir, "log.txt"))

                #print boostrap_out_dir
                shutil.move(simka_out_dir, boostrap_out_dir)
                #exit(0)

class ComputeBootstrapsStats():

    def __init__(self):
        if not os.path.exists(r_input_dir): os.makedirs(r_input_dir)
        if not os.path.exists(r_result_dir): os.makedirs(r_result_dir)


    def execute(self):
        data = {}

        for dir in glob.glob(os.path.join(boostrap_results_dir, "pass_*")):
            #print dir
            basename = os.path.basename(dir)
            dummy, percent, passID = basename.split("_")
            #print percent

            matrix_filenames = glob.glob(os.path.join(dir, "mat_*"))
            for matrix_filename in matrix_filenames:
                fields = os.path.basename(matrix_filename).split(".")[0].split("_")
                distance_name = fields[1] + "_" + fields[2]

                if not distance_name in data:
                    data[distance_name] = {}

                if not percent in data[distance_name]:
                    data[distance_name][percent] = []

                data[distance_name][percent].append(matrix_filename)

        input_filename_R = os.path.join(r_input_dir, "input_bootstrap.txt")
        input_R_file = open(input_filename_R, "w")
        #print data["abundance_braycurtis"]
        distance_name = "abundance_braycurtis"
        for percent, matrix_filenames  in data[distance_name].items():
            input_R_file.write(percent)
            for filename in matrix_filenames:
                input_R_file.write(" " + filename)
            input_R_file.write("\n")
            #print percent, matrix_filenames
            #print matrix_filenames
            #data[percent].append()
        input_R_file.close()

        os.system("Rscript subsampling_stats.r " + distance_name + " " + input_filename_R + " " + output_dir_temp)

#s = ComputeBootstraps()
#s.execute()

s = ComputeBootstrapsStats()
s.execute()