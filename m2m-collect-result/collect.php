<?php

$nH2h=30;
$nM2mList=array(0, 50, 100, 150, 200, 250);
// $nM2mList=array(0, 20, 50, 100, 150, 200, 230);
$schedulers=array(0, 1, 2, 3);
$nExec=10;
$fieldsIndex = array("type"=>0, "throughput"=>6, "fairness"=>7, "rx"=>12, "rxDelay"=>14, "tx"=>8, "txLoss"=>16);
$schedulerNameIndex = array(
		0=>array(0=>"M2M without Class", 1=>"M2M with Class"),
		1=>array(0=>"PF", 1=>"PF"),
		2=>array(0=>"RR", 1=>"RR"),
		3=>array(0=>"Lioumpas Alg. 2", 1=>"Lioumpas Alg. 2"),
);
$typeRespList=array("H2H All", "M2M Trigger", "M2M Regular All");
$schedulerRespList=array($schedulerNameIndex[0][0], $schedulerNameIndex[0][1], $schedulerNameIndex[1][0], $schedulerNameIndex[2][0], $schedulerNameIndex[3][0]);
$fieldRespList=array("throughput", "delayPercent", "fairness", "lossPercent");

$result=array();
foreach ($nM2mList as $nM2m) {
	$values = array();
	foreach ($schedulers as $sch) {
		$nM2mT = (int)floor($nM2m/3);
		$nM2mR = $nM2m - $nM2mT;
		for ($useClass = 0; $useClass < 2; $useClass++) {
			if ($sch > 0 && $useClass)
				continue;
			$schName = $schedulerNameIndex[$sch][$useClass];
			$valuesCount = 0;
			for ($exec = 0; $exec < $nExec; $exec++) {
				$fileName = "m2m-stats-geral-s($sch)-c($useClass)-h2h($nH2h)-m2mT($nM2mT)-m2mR($nM2mR)-$exec.csv";
				if (!file_exists($fileName)) {
// 					echo "Arquivo nao encontrado $fileName\n";
					continue;
				}
				$handle = fopen($fileName, "r");
				if (!$handle) {
// 					echo "Nao foi possivel abrir o arquivo $fileName\n";
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
					$delayPercent = ($tx > 0) ? ($rxDelay + $txLoss) / $tx : 0.0;
					
					if (!key_exists($schName, $values))
						$values[$schName] = array();
					$schValues=$values[$schName];
					if (!key_exists($type, $schValues)) {
						$schValues[$type] = array("throughput"=>0.0, "fairness"=> 0.0, "delayPercent"=>0.0, "lossPercent"=>0.0);
					}
					$schValues[$type]['throughput'] += $throughput;
					$schValues[$type]['fairness'] += $fairness;
					$schValues[$type]['delayPercent'] += $delayPercent;
					$schValues[$type]['lossPercent'] += $lossPercent;
					$values[$schName] = $schValues;
				}
				$valuesCount++;
			}

			if ($valuesCount == 0) {
// 				echo "Nenhum dado encontrado para o arquivo $fileName\n";
				continue;
			}

			$schValues=$values[$schName];
			foreach ($schValues as $type=>$row) {
				$schValues[$type]['throughput'] /= $valuesCount;
				$schValues[$type]['fairness'] /= $valuesCount;
				$schValues[$type]['delayPercent'] /= $valuesCount;
				$schValues[$type]['lossPercent'] /= $valuesCount;
			}
			$values[$schName] = $schValues;
		}
	}
	$result[$nM2m] = $values;
}

foreach ($typeRespList as $type) {
	foreach ($fieldRespList as $field) {
		$fileName="result-$type-$field.csv";
		$line = array("Number of Devices");
		$line = array_merge($line, $schedulerRespList);
		$content = implode(";", $line) . "\n";
		foreach ($nM2mList as $nM2m) {
			$line = array($nM2m+$nH2h);
			foreach ($schedulerRespList as $sch) {
				$line[] = $result[$nM2m][$sch][$type][$field];
			}
			$content .= implode(";", $line) . "\n";
		}
		file_put_contents($fileName, $content);
	}
}

?>