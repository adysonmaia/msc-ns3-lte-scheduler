<?php
function getStudentTDistribution($degree) {
	$table = array(0.0,
			12.70620474, 4.30265273, 3.18244631, 2.77644511, 2.57058184, 2.44691185, 2.36462425, 2.30600414, 2.26215716, 2.22813885,
			2.20098516, 2.17881283, 2.16036866, 2.14478669, 2.13144955, 2.11990530, 2.10981558, 2.10092204, 2.09302406, 2.08596345,
			2.07961385, 2.07387307, 2.06865761, 2.06389857, 2.05953856, 2.05552944, 2.05183052, 2.04840714, 2.04522964, 2.04227246,
			2.03951345, 2.03693334, 2.03451530, 2.03224451, 2.03010793, 2.0280940, 2.02619246, 2.02439416, 2.02269092, 2.02107539,
	);
	return $table[$degree];
}

function createImage($type, $field) {
	global $gnuPlotPath, $imageDataList, $separator, $nH2h, $nM2mList, $schedulerRespList;
	$fileName="result-$type-$field";
	$title = $imageDataList["type"][$type]["title"] . " \\n " . $imageDataList["field"][$field]["title"];
	$xLabel = "Weights";
	$yLabel = $imageDataList["field"][$field]["yLabel"];
	$imgW = 640;
	$imgH = 500;

	$command =
	"reset\n".
	"set datafile separator \"$separator\"\n".
	"set title \"$title\"\n".
	"set xlabel \"$xLabel\"\n".
	"set ylabel \"$yLabel\"\n".
	"set boxwidth 0.9 relative\n".
    "set style fill solid 1.0\n".
	"set key below box\n".
// 	"set key autotitle columnhead\n".
	"set grid y\n".
// 	"set style data boxerrorbars\n".
	"set style data yerrorlines\n".
// 	"set style increment user\n".
	"set terminal pngcairo size $imgW,$imgH enhanced font 'Verdana,10'\n".
	"set output \"$fileName.png\"\n".
// 	"plot \"$fileName.csv\" with errorbars\n";
	"plot \"$fileName.csv\" notitle \n";

	// 	file_put_contents("result-$type-$field.p", $command);

	$ph = popen($gnuPlotPath, 'w');
	fwrite($ph, $command);
	fclose($ph);
}

$gnuPlotPath = "/usr/bin/gnuplot";
if (PHP_OS == "Darwin")
	$gnuPlotPath = "/opt/local/bin/gnuplot";
$separator=";";
$nH2h=30;
$nM2mList=array(250);
$nMinPercentList=array("0.12", "0.24", "0.36", "0.48", "0.6", "0.72", "0.84", "0.96");
$schedulers=array(0);
$nExec=10;
$fieldsIndex = array("type"=>0, "throughput"=>6, "fairness"=>7, "rx"=>12, "rxDelay"=>14, "tx"=>8, "txLoss"=>16);
$schedulerNameIndex = array(
		0=>array(0=>"M2M without Class", 1=>"M2M with Class"),
		1=>array(0=>"PF", 1=>"PF"),
		2=>array(0=>"RR", 1=>"RR"),
		3=>array(0=>"Lioumpas Alg. 2", 1=>"Lioumpas Alg. 2"),
);
$typeRespList=array("H2H All", "M2M Trigger", "M2M Regular All", "M2M");
$schedulerRespList=array($schedulerNameIndex[0][1]);

$fieldRespList=array("throughput", "delayPercent", "fairness");
$imageDataList=array(
		"type"=>array(
				$typeRespList[0]=>array("title"=>"H2H Mixed Traffic"),
				$typeRespList[1]=>array("title"=>"M2M Event Driven Traffic"),
				$typeRespList[2]=>array("title"=>"M2M Time Driven Traffic"),
				$typeRespList[3]=>array("title"=>"M2M Traffic"),
		),
		"field"=>array(
				$fieldRespList[0] => array("title"=>"Transport block Throughput", "yLabel"=>"Throughput (kbps)"),
				$fieldRespList[1] => array("title"=>"QoS Safistaction", "yLabel"=>"Packets not meeting delay constraint (%)", "yMin"=>0, "yMax"=>100),
				$fieldRespList[2] => array("title"=>"Fairness", "yLabel"=>"Jain's fairness index", "yMin"=>0, "yMax"=>1.0),
		)
);

$useClass=1;
$scheduler=0;
$nM2mT = 83;
$nM2mR = 167;
$nRb = 3;
$delayWeight= 0.72;

$values=array();
foreach ($nMinPercentList as $nMinPercent) {
	$valuesCount = 0;
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
			$delayPercent = ($tx > 0) ? ($rxDelay + $txLoss) / $tx : 0.0;
		
			$lossPercent *= 100.0;
			$delayPercent *= 100.0;
		
			if (!key_exists($nMinPercent, $values))
				$values[$nMinPercent] = array();
			$schValues=$values[$nMinPercent];
			if (!key_exists($type, $schValues)) {
				$schValues[$type]["avg"] = array("throughput"=>0.0, "fairness"=> 0.0, "delayPercent"=>0.0, "lossPercent"=>0.0);
				$schValues[$type]["stdDvt"] = array("throughput"=>array(), "fairness"=>array(), "delayPercent"=>array(), "lossPercent"=>array());
			}
			$schValues[$type]["avg"]['throughput'] += $throughput;
			$schValues[$type]["avg"]['fairness'] += $fairness;
			$schValues[$type]["avg"]['delayPercent'] += $delayPercent;
			$schValues[$type]["avg"]['lossPercent'] += $lossPercent;
			$schValues[$type]["stdDvt"]['throughput'][] = $throughput;
			$schValues[$type]["stdDvt"]['fairness'][] = $fairness;
			$schValues[$type]["stdDvt"]['delayPercent'][] = $delayPercent;
			$schValues[$type]["stdDvt"]['lossPercent'][] = $lossPercent;
			$values[$nMinPercent] = $schValues;
		}
		$valuesCount++;
	}
	
	if ($valuesCount == 0) {
		continue;
	}
	
	$schValues=$values[$nMinPercent];
	foreach ($schValues as $type => $row) {
		$schValues[$type]["avg"]['throughput'] /= $valuesCount;
		$schValues[$type]["avg"]['fairness'] /= $valuesCount;
		$schValues[$type]["avg"]['delayPercent'] /= $valuesCount;
		$schValues[$type]["avg"]['lossPercent'] /= $valuesCount;
		$schValues[$type]["runs"] = $valuesCount;
		foreach ($schValues[$type]["stdDvt"] as $field => $fieldValues) {
			if ($valuesCount > 1) {
				$stdDvt = 0.0;
				$avg = $schValues[$type]["avg"][$field];
				foreach ($fieldValues as $fieldValue) {
					$stdDvt += ($fieldValue - $avg)*($fieldValue - $avg)/($valuesCount - 1);
				}
				$schValues[$type]["stdDvt"][$field] = $stdDvt;
			} else {
				$schValues[$type]["stdDvt"][$field] = 0.0;
			}
		}
	}
	$values[$nMinPercent] = $schValues;
}

foreach ($typeRespList as $type) {
	foreach ($fieldRespList as $field) {
		$fileName="result-$type-$field.csv";
		$content = "";
// 		$line = array("Delay Weight", "Avg", "Error Low", "Error High");
// 		$content .= implode($separator, $line) . "\n";
		
		foreach ($nMinPercentList as $nMinPercent) {
			$line = array();
			$avg = $values[$nMinPercent][$type]["avg"][$field];
			$stdDvt = $values[$nMinPercent][$type]["stdDvt"][$field];
			$runs = $values[$nMinPercent][$type]["runs"];
			$error = getStudentTDistribution($runs - 1)*sqrt($stdDvt/$runs);
			$errorLow = $avg - $error;
			$errorHigh = $avg + $error;
			$line[] = $nMinPercent;
			$line[] = $avg;
			$line[] = $errorLow;
			$line[] = $errorHigh;
			$content .= implode($separator, $line) . "\n";
		}
		
		file_put_contents($fileName, $content);
		createImage($type, $field);
	}
}

?>
