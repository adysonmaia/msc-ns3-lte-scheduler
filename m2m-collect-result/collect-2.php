<?php
$nH2h=30;
$nM2mR=167;
$nM2mT=83;
$scheduler=0;
$nExec=1;
$useClass=1;
$bandwidth=25;
//$delayWeightList=array("0","0.3","0.5","0.7","0.8","0.9","1");
$delayWeightList=array("1", "0.8", "0.75", "0.72", "0.7", "0.5", "0.3", "0");
$nRbPerM2mList=array(3);
// $nMinM2mPerTtiList=array(2,3,4);
$nMinM2mPerTtiList=array(4);

$separator=";";
$fieldsIndex = array("type"=>0, "throughput"=>6, "fairness"=>7, "rx"=>12, "rxDelay"=>14, "tx"=>8, "txLoss"=>16);
$typeRespList=array("M2M", "M2M Trigger", "M2M Regular All");
$fieldResp="delayPercent";

$result=array();
foreach ($nRbPerM2mList as $nRb) {
	foreach ($nMinM2mPerTtiList as $nMinM2m) {
		$nMinPercent = round($nRb*$nMinM2m/(double)$bandwidth, 2);
		foreach ($delayWeightList as $delayWeight) {
			$valuesCount = 0;
			$result[$nRb][$nMinM2m][$delayWeight] = array();
			for ($exec = 0; $exec < $nExec; $exec++) {
				$fileName = "m2m-stats-geral-s($scheduler)-c($useClass)-h2h($nH2h)-m2mT($nM2mT)-m2mR($nM2mR)-$exec-$delayWeight-$nRb-$nMinPercent.csv";
				if (!file_exists($fileName)) {
					echo "Arquivo nao encontrado $fileName\n";
					continue;
				}
				$handle = fopen($fileName, "r");
				if (!$handle) {
					continue;
				}
				fgets($handle); // first line - header
				while (($line = fgets($handle)) !== false) {
					$fields = explode(";", $line);
					$type = trim($fields[$fieldsIndex['type']]);
					$throughput = (double)$fields[$fieldsIndex['throughput']];
					$fairness = (double)$fields[$fieldsIndex['fairness']];
					$rx = (int)$fields[$fieldsIndex['rx']];
					$rxDelay = (int)$fields[$fieldsIndex['rxDelay']];
					$tx = (int)$fields[$fieldsIndex['tx']];
					$txLoss = (int)$fields[$fieldsIndex['txLoss']];
					$lossPercent = ($tx > 0) ? $txLoss / $tx : 0.0;
					$delayPercent = ($tx > 0) ? ($rxDelay + $txLoss) / (double)$tx : 0.0;
					
					if (!key_exists($type, $result[$nRb][$nMinM2m][$delayWeight])) {
						$result[$nRb][$nMinM2m][$delayWeight][$type]["avg"] = array("throughput"=>0.0, "fairness"=> 0.0, "delayPercent"=>0.0, "lossPercent"=>0.0);
						$result[$nRb][$nMinM2m][$delayWeight][$type]["stdDvt"] = array("throughput"=>array(), "fairness"=>array(), "delayPercent"=>array(), "lossPercent"=>array());
					}
					$result[$nRb][$nMinM2m][$delayWeight][$type]["avg"]['throughput'] += $throughput;
					$result[$nRb][$nMinM2m][$delayWeight][$type]["avg"]['fairness'] += $fairness;
					$result[$nRb][$nMinM2m][$delayWeight][$type]["avg"]['delayPercent'] += $delayPercent;
					$result[$nRb][$nMinM2m][$delayWeight][$type]["avg"]['lossPercent'] += $lossPercent;
					$result[$nRb][$nMinM2m][$delayWeight][$type]["stdDvt"]['throughput'][] = $throughput;
					$result[$nRb][$nMinM2m][$delayWeight][$type]["stdDvt"]['fairness'][] = $fairness;
					$result[$nRb][$nMinM2m][$delayWeight][$type]["stdDvt"]['delayPercent'][] = $delayPercent;
					$result[$nRb][$nMinM2m][$delayWeight][$type]["stdDvt"]['lossPercent'][] = $lossPercent;
				}
				$valuesCount++;
			}
			
			if ($valuesCount == 0) {
				continue;
			}
			
			$values = $result[$nRb][$nMinM2m][$delayWeight];
			foreach ($values as $type => $row) {
				$values[$type]["avg"]['throughput'] /= $valuesCount;
				$values[$type]["avg"]['fairness'] /= $valuesCount;
				$values[$type]["avg"]['delayPercent'] /= $valuesCount;
				$values[$type]["avg"]['lossPercent'] /= $valuesCount;
				$values[$type]["runs"] = $valuesCount;
				foreach ($values[$type]["stdDvt"] as $field => $fieldValues) {
					if ($valuesCount > 1) {
						$stdDvt = 0.0;
						$avg = $values[$type]["avg"][$field];
						foreach ($fieldValues as $fieldValue) {
							$stdDvt += ($fieldValue - $avg)*($fieldValue - $avg)/($valuesCount - 1);
						}
						$values[$type]["stdDvt"][$field] = $stdDvt;
					} else {
						$values[$type]["stdDvt"][$field] = 0.0;
					}
				}
			}
			$result[$nRb][$nMinM2m][$delayWeight] = $values;
		}
	}
}

foreach ($typeRespList as $type) {
	foreach ($nRbPerM2mList as $nRb) {
		$fileName = "result-$type-$nRb.csv";
		$line = array("");
		foreach ($delayWeightList as $delayWeight) {
			$line[] = $delayWeight;
		}
		$content = implode($separator, $line) . "\n";

		foreach ($nMinM2mPerTtiList as $nMinM2m) {
			$nMinPercent = round($nRb*$nMinM2m/(double)$bandwidth, 2);
			$line = array($nMinPercent);
			foreach ($delayWeightList as $delayWeight) {
				$line[] = $result[$nRb][$nMinM2m][$delayWeight][$type]["avg"][$fieldResp];
			}
			$content .= implode($separator, $line) . "\n";
		}
		file_put_contents($fileName, $content);
	}
}

?>