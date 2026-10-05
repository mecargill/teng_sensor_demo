from matplotlib import pyplot as plt
import numpy as np
import os

csv_paths = {
    "adc_data/adc_data_charge_amp_10nF_1p6Mohm.csv" : "Charge Amp 10nF - no env det",
    "adc_data/adc_data_charge_amp_150nF_1p6Mohm.csv" : "Charge Amp 150nF - no env det",
    "adc_data/adc_data_charge_amp_10nF_1p6Mohm_env_10uF_90kohm.csv" : "Charge Amp 10nF + Env Det 10uF w 90kohm bleed",
    "adc_data/adc_data_charge_amp_10nF_1p6Mohm_env_10uF_open.csv" : "Charge Amp 10nF + Env Det 10uF w no bleed",
    "adc_data/only_env_det_SENSITIVE.csv" : "only env det",
    "adc_data/adc_data_charge_amp_10nF_1p6Mohm_env_10nF_open.csv" : "Charge Amp 10nF + Env Det 10nF w no bleed"

}
"""{
"adc_data/adc_data_running_off_battery_just_noise.csv" : "What freq is the noise?, 60Hz?"
}"""
"""{
    "adc_data/actual_teng_rec_CA10pF_env10u.csv" : "Actual Teng Data"
}"""


specgram = False
"""{
    "adc_data/adc_data_charge_amp_10nF_1p6Mohm.csv" : "Charge Amp 10nF - no env det",
    "adc_data/adc_data_charge_amp_150nF_1p6Mohm.csv" : "Charge Amp 150nF - no env det",
    "adc_data/adc_data_charge_amp_10nF_1p6Mohm_env_10uF_90kohm.csv" : "Charge Amp 10nF + Env Det 10uF w 90kohm bleed",
    "adc_data/adc_data_charge_amp_10nF_1p6Mohm_env_10uF_open.csv" : "Charge Amp 10nF + Env Det 10uF w no bleed",
    "adc_data/only_env_det_SENSITIVE.csv" : "only env det",
    "adc_data/adc_data_charge_amp_10nF_1p6Mohm_env_10nF_open.csv" : "Charge Amp 10nF + Env Det 10nF w no bleed"

}"""


for csv_path in csv_paths.keys():
    with open(csv_path, 'r') as file:
        flat_data = file.read().replace('\n', ',')

    # Parse into a flat NumPy array
    arr = np.fromstring(flat_data, sep=',', dtype=int)
    x = range(len(arr))
    plt.figure()
    plt.plot(x, arr)
    plt.ylabel("ADC Output (~0-3V, 0-4096)")
    plt.title(f"{csv_paths[csv_path]}")
    if specgram:
        plt.figure()
        plt.specgram(arr - np.mean(arr), Fs=2000)
plt.show()