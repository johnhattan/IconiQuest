package;

import openfl.*;
import openfl.display.*;
import openfl.events.*;
import openfl.net.*;
import openfl.geom.*;

import motion.easing.*;
import motion.*;

using ZManager;
using CZHelpers;

import Entity;

class WhistleActor extends Actor
{
	public override function onTap(numTaps:Int = 1)
	{
		super.onTap(numTaps);

		if (numTaps == 2)
		{
			Sounds.play('sndWhistle');

			var here:Location = World.worldMain.locationCurrent;
			var placeholder:Actor = here.actor('glyphDinoTriceratopsPlaceholder');
			if (placeholder != null)
			{
				var actorTriceratops:Actor = World.worldMain.actor('glyphDinoTriceratops');
				here.takeActor(actorTriceratops);
				actorTriceratops.x = placeholder.x;
				actorTriceratops.y = placeholder.y;
				actorTriceratops.refresh(1);
				
				var actorTyrannosaur:Actor = here.actor('glyphDinoTyrannosaur');
				if (actorTyrannosaur != null)
				{	// scare him away
					var actorTyrannosaurPlaceholder:Actor = World.worldMain.actor('glyphDinoTyrannosaurPlaceholder');
					var there:Location = actorTyrannosaurPlaceholder.locationParent;
					there.takeActor(actorTyrannosaur);
					actorTyrannosaur.x = actorTyrannosaurPlaceholder.x;
					actorTyrannosaur.y = actorTyrannosaurPlaceholder.y;
					actorTyrannosaur.refresh(1, 1.25);
				}
			}
		}
	}
}

class ApeManActor extends Actor
{
	public override function onPlace()
	{
		super.onPlace();

		if (locationParent.actor('glyphRocksToggled') != null)
			propertySet('note', 'glyphEmotionHappy');
		else
			propertySet('note', 'glyphEmotionAngry');
	}
}

class VolcanoArrow extends Actor
{
	public override function onPlace()
	{
		super.onPlace();
		if (worldParent.actor('glyphRocksToggled') != null)
		{	// happy happy
			locationParent.actor('glyphApeman3-1').hidden = locationParent.actor('glyphApeman3-2').hidden = true;
			locationParent.actor('glyphApeman3-3').hidden = locationParent.actor('glyphApeman3-4').hidden = false;
			propertySet('relocaterDest', '3-3');
			propertySetAsFloat('relocaterDestTime', 0.75);
		}
		else
		{	// you have a problem
			locationParent.actor('glyphApeman3-1').hidden = locationParent.actor('glyphApeman3-2').hidden = false;
			locationParent.actor('glyphApeman3-3').hidden = locationParent.actor('glyphApeman3-4').hidden = true;
			propertySet('relocaterDest', 'locDeathVolcano');
			propertySetAsFloat('relocaterDestTime', 0);
		}
	}
}

class MainWorld extends World
{
	private var recalcLocations:Array<Location>;
	private var actorScore:Actor;

	public function calcScore():Int
	{
		var score:Int = 1000;
		var loc:Location = location('inventory.default');

		if (loc != null)
			for (actor in loc.actors)
			{
				switch (actor.name)
				{
					case 'glyphUmbrella':
						score -= 25;
					case 'glyphUmbrellaToggled':
						score -= 50;

					case 'glyphKey':
						score -= 25;
						
					case 'glyphCorkscrew':
						score -= 50;

					case 'glyphGun':
						score -= 50;
					case 'glyphGunToggled':
						score -= 100;
					case 'glyphBullet':
						score -= 50;

					case 'glyphMatch':
						score -= 50;
					case 'glyphMatchToggled':
						score -= 75;

					case 'glyphWindup':
						score -= 100;

					case 'glyphBook':
						score -= 100;
					case 'glyphBookToggled':
						score -= 100;

					case 'glyphKnife':
						score -= 100;

					case 'glyphGlass':
						score -= 100;
					case 'glyphGlassToggled':
						score -= 150;

					case 'glyphCandle':
						score -= 75;
					case 'glyphCandleToggled':
						score -= 125;

					case 'glyphMap':
						score -= 75;

					case 'glyphCombination':
						score -= 75;

					case 'glyphMoney':
						score -= 200;

					case 'glyphBottle':
						score -= 100;
					case 'glyphBottleToggled':
						score -= 100;
				}
			}

		return score;
	}

	public override function onInit()
	{
		recalcLocations = [ location('partWin'), location('partLose') ];

		var worldSettings:World = World.world('settings');
		actorScore = (worldSettings != null) ? worldSettings.actor('score') : null;
	}

	public override function onChanged()
	{
		if (recalcLocations.indexOf(locationCurrent) != -1)
			actorScore.glyph = '#'+Std.string(calcScore())+'#';
	}
}
